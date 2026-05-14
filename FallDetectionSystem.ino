/*
 * ============================================================
 *  FallDetectionSystem.ino — Main Sketch
 *
 *  Hardware:
 *    ESP32 Dev Module
 *    MPU6050 Accelerometer + Gyroscope (I2C)
 *    LED  on GPIO2  (with 220Ω series resistor to GND)
 *    Buzzer on GPIO4 (active buzzer, + to pin, - to GND)
 *
 *  Libraries required (install via Library Manager):
 *    - "MPU6050" by Electronic Cats
 *    - "Wire"    (built-in, no install needed)
 *
 *  Wiring summary:
 *    MPU6050 VCC  → ESP32 3.3V
 *    MPU6050 GND  → ESP32 GND
 *    MPU6050 SDA  → ESP32 GPIO21
 *    MPU6050 SCL  → ESP32 GPIO22
 *    MPU6050 AD0  → GND  (I2C address = 0x68)
 *    LED    (+)   → GPIO2 → [220Ω] → GND
 *    Buzzer (+)   → GPIO4, (-) → GND
 *
 *  All tunable parameters are in config.h
 * ============================================================
 */

#include <Wire.h>
#include <MPU6050.h>
#include "config.h"
#include "wifi_alert.h"

// ── MPU6050 Object ───────────────────────────────────────────
MPU6050 mpu;

// ── Detection State Machine ───────────────────────────────────
//   IDLE      → monitoring normally
//   FREEFALL  → free-fall phase detected, waiting for impact
//   ALERT     → fall confirmed, alert active
//   COOLDOWN  → alert finished, ignoring inputs briefly

enum DetectionState {
  STATE_IDLE,
  STATE_FREEFALL,
  STATE_ALERT,
  STATE_COOLDOWN
};

DetectionState currentState   = STATE_IDLE;
unsigned long  stateEnteredAt = 0;  // millis() when we entered current state

// ── Sensor Data ───────────────────────────────────────────────
float ax, ay, az;     // Acceleration in g
float gx, gy, gz;     // Angular velocity in °/s
float magnitude;      // |A| = sqrt(ax²+ay²+az²)

// ─────────────────────────────────────────────────────────────
//  setup()
// ─────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);

  printBanner();

  // Setup output pins
  pinMode(LED_PIN,    OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_PIN,    LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // Initialize I2C with explicit SDA/SCL pins
  Wire.begin(MPU_SDA_PIN, MPU_SCL_PIN);

  // Initialize MPU6050
  Serial.println("[INIT] Initializing MPU6050...");

  // Ping the I2C address directly — works with all MPU6050 clones
  Wire.beginTransmission(0x68);
  bool mpuFound = (Wire.endTransmission() == 0);

  if (!mpuFound) {
    Serial.println("[ERROR] MPU6050 not found! Check wiring:");
    Serial.println("        VCC → 3.3V  |  GND → GND");
    Serial.println("        SDA → GPIO21  |  SCL → GPIO22");
    Serial.println("        AD0 → GND");
    // Blink LED rapidly to signal hardware error
    while (true) {
      digitalWrite(LED_PIN, HIGH); delay(100);
      digitalWrite(LED_PIN, LOW);  delay(100);
    }
  }

  mpu.initialize();
  Serial.println("[INIT] MPU6050 connected OK (address 0x68)");

  // Set sensitivity ranges
  // ±2g  = most sensitive accel range (best for fall detection)
  // ±250°/s = standard gyro range
  mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_2);
  mpu.setFullScaleGyroRange(MPU6050_GYRO_FS_250);

  // Connect WiFi if enabled
  connectWiFi();

  // Startup confirmation blink
  blinkLED(3, 150);
  Serial.println("[INIT] System ready. Monitoring for falls...\n");

  #if VERBOSE_SERIAL
    Serial.println("  Time(ms)  |  Ax(g)  Ay(g)  Az(g)  |  |A|(g)  |  State");
    Serial.println("  ----------|-------------------------|---------|--------");
  #endif
}

// ─────────────────────────────────────────────────────────────
//  loop()
// ─────────────────────────────────────────────────────────────
void loop() {
  unsigned long now = millis();

  // ── 1. Read Sensor ──────────────────────────────────────────
  readMPU6050();

  // ── 2. Serial Output ────────────────────────────────────────
  #if VERBOSE_SERIAL
    printSensorData(now);
  #endif

  // ── 3. State Machine ────────────────────────────────────────
  switch (currentState) {

    // ── IDLE: Normal monitoring ─────────────────────────────
    case STATE_IDLE:
      if (magnitude < FREE_FALL_THRESHOLD) {
        // Phase 1 — free-fall detected
        currentState   = STATE_FREEFALL;
        stateEnteredAt = now;
        Serial.println("\n>>> [PHASE 1] FREE-FALL DETECTED!");
        Serial.print  ("    |A| = "); Serial.print(magnitude, 3);
        Serial.print  ("g  (threshold < "); Serial.print(FREE_FALL_THRESHOLD);
        Serial.println("g)");
      }
      break;

    // ── FREEFALL: Waiting for impact within window ──────────
    case STATE_FREEFALL:
      if (magnitude > IMPACT_THRESHOLD) {
        // Phase 2 — impact within the window → FALL CONFIRMED
        Serial.println(">>> [PHASE 2] IMPACT DETECTED!");
        Serial.print  ("    |A| = "); Serial.print(magnitude, 3);
        Serial.print  ("g  (threshold > "); Serial.print(IMPACT_THRESHOLD);
        Serial.println("g)");

        // Optional: posture check
        #if ENABLE_POSTURE_CHECK
          if (abs(az) < POSTURE_LYING_THRESHOLD) {
            confirmFall();
          } else {
            Serial.println(">>> [POSTURE] Person upright — likely false alarm. Ignoring.");
            currentState = STATE_IDLE;
          }
        #else
          confirmFall();
        #endif

      } else if (now - stateEnteredAt > FALL_WINDOW_MS) {
        // Timed out — no impact followed free-fall
        Serial.println(">>> [TIMEOUT] No impact within window. Resetting to IDLE.");
        currentState = STATE_IDLE;
      }
      break;

    // ── ALERT: Fall confirmed, running alert sequence ────────
    case STATE_ALERT:
      runAlert(now);
      break;

    // ── COOLDOWN: Alert done, ignoring new events briefly ────
    case STATE_COOLDOWN:
      if (now - stateEnteredAt > COOLDOWN_PERIOD_MS) {
        currentState = STATE_IDLE;
        Serial.println("[INFO] Cooldown complete. Resuming monitoring.\n");
        #if VERBOSE_SERIAL
          Serial.println("  Time(ms)  |  Ax(g)  Ay(g)  Az(g)  |  |A|(g)  |  State");
          Serial.println("  ----------|-------------------------|---------|--------");
        #endif
      }
      break;
  }

  delay(SAMPLE_INTERVAL_MS);
}

// ─────────────────────────────────────────────────────────────
//  confirmFall() — called when both phases are confirmed
// ─────────────────────────────────────────────────────────────
void confirmFall() {
  Serial.println("\n!!! ================================= !!!");
  Serial.println("!!!         FALL CONFIRMED            !!!");
  Serial.println("!!!  Activating buzzer and LED alert  !!!");
  Serial.println("!!! ================================= !!!\n");

  currentState = STATE_ALERT;

  sendTelegramAlert();         // blocks until HTTP response arrives
  stateEnteredAt = millis();   // start alert timer AFTER Telegram returns
}

// ─────────────────────────────────────────────────────────────
//  runAlert() — called every loop() tick while in STATE_ALERT
//  Blinks LED and beeps buzzer for ALERT_DURATION_MS total.
// ─────────────────────────────────────────────────────────────
void runAlert(unsigned long now) {
  unsigned long elapsed = now - stateEnteredAt;

  if (elapsed < ALERT_DURATION_MS) {
    // Blink/beep based on elapsed time
    bool alertOn = ((elapsed / ALERT_BLINK_INTERVAL_MS) % 2 == 0);
    digitalWrite(LED_PIN,    alertOn ? HIGH : LOW);
    digitalWrite(BUZZER_PIN, alertOn ? HIGH : LOW);
  } else {
    // Alert duration finished
    digitalWrite(LED_PIN,    LOW);
    digitalWrite(BUZZER_PIN, LOW);
    currentState   = STATE_COOLDOWN;
    stateEnteredAt = now;
    Serial.println("[INFO] Alert finished. Entering cooldown...");
  }
}

// ─────────────────────────────────────────────────────────────
//  readMPU6050() — reads sensor and updates global variables
// ─────────────────────────────────────────────────────────────
void readMPU6050() {
  int16_t ax_raw, ay_raw, az_raw;
  int16_t gx_raw, gy_raw, gz_raw;

  mpu.getMotion6(&ax_raw, &ay_raw, &az_raw,
                 &gx_raw, &gy_raw, &gz_raw);

  // At ±2g range:    1g     = 16384 LSB
  // At ±250°/s range: 1°/s = 131 LSB
  ax = ax_raw / 16384.0f;
  ay = ay_raw / 16384.0f;
  az = az_raw / 16384.0f;
  gx = gx_raw / 131.0f;
  gy = gy_raw / 131.0f;
  gz = gz_raw / 131.0f;

  magnitude = sqrt(ax*ax + ay*ay + az*az);
}

// ─────────────────────────────────────────────────────────────
//  printSensorData() — formatted Serial Monitor output
// ─────────────────────────────────────────────────────────────
void printSensorData(unsigned long now) {
  // State label
  const char* stateLabel;
  switch (currentState) {
    case STATE_IDLE:      stateLabel = "IDLE    "; break;
    case STATE_FREEFALL:  stateLabel = "FREEFALL"; break;
    case STATE_ALERT:     stateLabel = "ALERT   "; break;
    case STATE_COOLDOWN:  stateLabel = "COOLDOWN"; break;
    default:              stateLabel = "UNKNOWN "; break;
  }

  Serial.print("  ");
  Serial.print(now);
  // Pad to column width
  if (now < 10000)    Serial.print("  ");
  else if (now < 100000) Serial.print(" ");

  Serial.print("  |  ");
  Serial.print(ax,    3); Serial.print("  ");
  Serial.print(ay,    3); Serial.print("  ");
  Serial.print(az,    3);
  Serial.print("  |  ");
  Serial.print(magnitude, 3);
  Serial.print("   |  ");
  Serial.println(stateLabel);
}

// ─────────────────────────────────────────────────────────────
//  blinkLED() — blink n times with given interval ms
// ─────────────────────────────────────────────────────────────
void blinkLED(int times, int intervalMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH); delay(intervalMs);
    digitalWrite(LED_PIN, LOW);  delay(intervalMs);
  }
}

// ─────────────────────────────────────────────────────────────
//  printBanner() — startup message
// ─────────────────────────────────────────────────────────────
void printBanner() {
  Serial.println();
  Serial.println("  =============================================");
  Serial.println("       FALL DETECTION SYSTEM — ESP32          ");
  Serial.println("       MPU6050 + LED + Buzzer                 ");
  Serial.println("  =============================================");
  Serial.print  ("  Free-fall threshold : < "); Serial.print(FREE_FALL_THRESHOLD); Serial.println(" g");
  Serial.print  ("  Impact threshold    : > "); Serial.print(IMPACT_THRESHOLD);    Serial.println(" g");
  Serial.print  ("  Detection window    :   "); Serial.print(FALL_WINDOW_MS);      Serial.println(" ms");
  Serial.print  ("  Alert duration      :   "); Serial.print(ALERT_DURATION_MS);   Serial.println(" ms");
  Serial.print  ("  Cooldown period     :   "); Serial.print(COOLDOWN_PERIOD_MS);  Serial.println(" ms");
  Serial.print  ("  WiFi alert          :   ");
  #if ENABLE_WIFI_ALERT
    Serial.println("ENABLED");
  #else
    Serial.println("disabled");
  #endif
  Serial.println("  =============================================");
  Serial.println();
}
