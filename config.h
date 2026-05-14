/*
 * ============================================================
 *  config.h — Fall Detection System Configuration
 *  Edit this file to tune sensitivity and hardware pins.
 * ============================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

// ── GPIO Pin Assignments ──────────────────────────────────────
#define LED_PIN         2       // GPIO2  → LED anode (via 220Ω resistor)
#define BUZZER_PIN      4       // GPIO4  → Buzzer positive terminal
#define MPU_SDA_PIN     21      // GPIO21 → MPU6050 SDA
#define MPU_SCL_PIN     22      // GPIO22 → MPU6050 SCL

// ── Fall Detection Thresholds ─────────────────────────────────
// Free-fall: total acceleration drops below this value (in g)
// Lower  = more sensitive to free-fall, but more false positives
// Higher = less sensitive to free-fall
#define FREE_FALL_THRESHOLD     0.5f    // g  (normal = 1.0g)

// Impact: total acceleration spikes above this value (in g)
// Lower  = triggers more easily on impact
// Higher = requires a harder impact to trigger
#define IMPACT_THRESHOLD        2.5f    // g  (normal = 1.0g)

// Time window between free-fall and impact to count as one fall (ms)
// Too short = may miss slow falls. Too long = may catch unrelated motions.
#define FALL_WINDOW_MS          500UL   // milliseconds

// ── Alert Settings ────────────────────────────────────────────
// How long the buzzer and LED stay active after a fall (ms)
#define ALERT_DURATION_MS       4000UL  // 4 seconds

// Cooldown after an alert — prevents repeated triggering (ms)
#define COOLDOWN_PERIOD_MS      10000UL // 10 seconds

// Blink/beep interval during alert (ms)
#define ALERT_BLINK_INTERVAL_MS 200UL   // 200ms on / 200ms off

// ── Sampling Rate ─────────────────────────────────────────────
// How often the sensor is read (ms). Lower = faster response.
#define SAMPLE_INTERVAL_MS      50UL    // 50ms = 20 Hz

// ── Posture Check (Post-Fall Verification) ────────────────────
// After impact, if |az| < this value, person is lying down (real fall)
// If az ≈ 1.0g after impact, person stood up quickly (likely false alarm)
#define POSTURE_LYING_THRESHOLD 0.4f    // g — az near 0 = horizontal

// Enable posture check? (1 = yes, stricter; 0 = no, trigger on impact alone)
#define ENABLE_POSTURE_CHECK    0       // Set to 1 for stricter detection

// ── WiFi Alert Settings ───────────────────────────────────────
// Set ENABLE_WIFI_ALERT to 1 to enable Telegram notifications
#define ENABLE_WIFI_ALERT       1       // 0 = disabled, 1 = enabled

// Fill these in only if ENABLE_WIFI_ALERT = 1
#define WIFI_SSID               "YOUR_WIFI_SSID"
#define WIFI_PASSWORD           "YOUR_WIFI_PASSWORD"
#define TELEGRAM_BOT_TOKEN      "YOUR_BOT_TOKEN"        // from @BotFather on Telegram
#define TELEGRAM_CHAT_ID        "YOUR_CHAT_ID"          // from @userinfobot on Telegram

// ── Debug Output ──────────────────────────────────────────────
// Set to 1 to print detailed sensor data every sample (noisy)
// Set to 0 to only print fall events (clean output)
#define VERBOSE_SERIAL          1

#endif // CONFIG_H
