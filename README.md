# Fall Detection System — ESP32 + MPU6050

An embedded fall detection system using an ESP32 microcontroller and MPU6050 IMU sensor. Detects falls using a two-phase algorithm (free-fall → impact), triggers a buzzer and LED alert, and sends an instant Telegram notification via WiFi.

---

## Features

- Two-phase fall detection (free-fall + impact) to minimize false positives
- Real-time accelerometer + gyroscope monitoring at 20 Hz
- Buzzer and LED alert on fall confirmation
- Telegram bot notification via WiFi (optional)
- Optional posture check — verifies person is lying down after impact
- Configurable thresholds, timing, and pins via `config.h`
- 4-state machine: `IDLE → FREEFALL → ALERT → COOLDOWN`

---

## Hardware

| Component | Details |
|---|---|
| Microcontroller | ESP32 Dev Module |
| IMU Sensor | MPU6050 (Accelerometer + Gyroscope) |
| LED | Standard 5mm LED with 220Ω series resistor |
| Buzzer | Active buzzer |

### Wiring

| MPU6050 Pin | ESP32 Pin |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| AD0 | GND (sets I²C address to 0x68) |

| Component | ESP32 Pin |
|---|---|
| LED (+) | GPIO 2 → 220Ω → GND |
| Buzzer (+) | GPIO 4 |
| Buzzer (−) | GND |

---

## How It Works

The system uses a two-phase detection approach:

**Phase 1 — Free-fall:** The MPU6050 reads near-zero acceleration (< 0.5g) indicating the person is airborne. During true free-fall, gravity acts equally on all parts of the sensor, so the measured magnitude drops close to 0g.

**Phase 2 — Impact:** Within a 500ms window after free-fall, a sudden acceleration spike (> 2.5g) is detected as the body hits the ground.

Both phases must occur in sequence to confirm a fall — this eliminates false positives from everyday activities like walking or running.

```
IDLE ──(|A| < 0.5g)──► FREEFALL ──(|A| > 2.5g within 500ms)──► ALERT ──(4s)──► COOLDOWN ──(10s)──► IDLE
```

---

## Setup

### 1. Install Libraries

In Arduino IDE, go to **Tools → Manage Libraries** and install:

- `MPU6050` by Electronic Cats

`Wire` is built-in and requires no installation.

### 2. Select Board

- Board: **ESP32 Dev Module**
- Upload Speed: 115200
- Port: whichever COM port your ESP32 appears on

### 3. Configure

Open `config.h` and set your parameters:

```cpp
// Fall detection thresholds
#define FREE_FALL_THRESHOLD   0.5f    // g — below this = free-fall detected
#define IMPACT_THRESHOLD      2.5f    // g — above this = impact detected
#define FALL_WINDOW_MS        500UL   // ms — max time between free-fall and impact

// Alert duration and cooldown
#define ALERT_DURATION_MS     4000UL  // ms — how long buzzer/LED stay on
#define COOLDOWN_PERIOD_MS    10000UL // ms — wait before monitoring resumes
```

### 4. Enable Telegram Alerts (Optional)

In `config.h`, set:

```cpp
#define ENABLE_WIFI_ALERT     1
#define WIFI_SSID             "your_wifi_name"
#define WIFI_PASSWORD         "your_wifi_password"
#define TELEGRAM_BOT_TOKEN    "your_bot_token"   // from @BotFather
#define TELEGRAM_CHAT_ID      "your_chat_id"     // from @userinfobot
```

To get a bot token: open Telegram → search `@BotFather` → `/newbot`  
To get your chat ID: open Telegram → search `@userinfobot` → `/start`

### 5. Upload

Click **Upload** in Arduino IDE. Open Serial Monitor at **115200 baud** to view live output.

---

## Threshold Justification

| Parameter | Value | Basis |
|---|---|---|
| Free-fall threshold | 0.5g | During free-fall, proper acceleration ≈ 0g; 0.5g allows for partial/angled falls (Kangas et al., 2008) |
| Impact threshold | 2.5g | Real fall impacts range 2g–8g; normal walking peaks at ~1.3g (Bourke et al., 2007) |
| Fall window | 500ms | Physics: falling from 1m takes ~0.45s (t = √(2h/g)); 500ms adds a small buffer |

---

## Serial Monitor Output

```
  =============================================
       FALL DETECTION SYSTEM — ESP32
       MPU6050 + LED + Buzzer
  =============================================
  Free-fall threshold : < 0.5 g
  Impact threshold    : > 2.5 g
  Detection window    : 500 ms
  ...

>>> [PHASE 1] FREE-FALL DETECTED!
    |A| = 0.231g  (threshold < 0.5g)
>>> [PHASE 2] IMPACT DETECTED!
    |A| = 3.847g  (threshold > 2.5g)

!!! ================================= !!!
!!!         FALL CONFIRMED            !!!
!!!  Activating buzzer and LED alert  !!!
!!! ================================= !!!

[Telegram] Alert sent successfully!
[INFO] Alert finished. Entering cooldown...
[INFO] Cooldown complete. Resuming monitoring.
```

---

## Project Structure

```
FallDetectionSystem/
├── FallDetectionSystem.ino   # Main sketch — setup, loop, state machine
├── config.h                  # All tunable parameters and pin assignments
├── wifi_alert.h              # WiFi connection and Telegram alert functions
└── diagrams.html             # Hardware and workflow diagrams (open in browser)
```

---

## References

- Kangas, M. et al. (2008). *Determination of simple thresholds for accelerometry-based parameters for fall detection.* EMBC.
- Bourke, A.K. & Lyons, G.M. (2008). *A threshold-based fall-detection algorithm using a bi-axial gyroscope sensor.* Medical Engineering & Physics.
