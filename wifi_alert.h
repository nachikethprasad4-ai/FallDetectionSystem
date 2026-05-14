/*
 * ============================================================
 *  wifi_alert.h — WiFi + Telegram Alert Module
 *  Only active when ENABLE_WIFI_ALERT = 1 in config.h
 * ============================================================
 */

#ifndef WIFI_ALERT_H
#define WIFI_ALERT_H

#include "config.h"

#if ENABLE_WIFI_ALERT

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// ─────────────────────────────────────────────────────────────
// connectWiFi()
// Connects to the WiFi network defined in config.h.
// Blocks until connected or times out after 15 seconds.
// ─────────────────────────────────────────────────────────────
void connectWiFi() {
  Serial.print("[WiFi] Connecting to: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startAttempt > 15000) {
      Serial.println("\n[WiFi] Connection TIMED OUT. Continuing without WiFi.");
      return;
    }
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("[WiFi] Connected! IP: ");
  Serial.println(WiFi.localIP());
}

// ─────────────────────────────────────────────────────────────
// sendTelegramAlert()
// Sends a Telegram message via the Bot API over HTTPS.
// Call this inside triggerAlert() in the main sketch.
// ─────────────────────────────────────────────────────────────
void sendTelegramAlert() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Not connected — skipping Telegram alert.");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();  // Skip SSL certificate verification (simple approach)

  HTTPClient http;

  // Build the Telegram Bot API URL
  String url = String("https://api.telegram.org/bot") +
               TELEGRAM_BOT_TOKEN +
               "/sendMessage?chat_id=" +
               TELEGRAM_CHAT_ID +
               "&text=" +
               "%F0%9F%9A%A8%20FALL%20DETECTED!%20%0A"   // 🚨 FALL DETECTED!
               "Please%20check%20on%20the%20person%20immediately.%0A"
               "%F0%9F%93%8D%20Device%3A%20ESP32%20Fall%20Detector";

  http.begin(client, url);
  int httpCode = http.GET();

  if (httpCode == 200) {
    Serial.println("[Telegram] Alert sent successfully!");
  } else {
    Serial.print("[Telegram] Failed. HTTP code: ");
    Serial.println(httpCode);
  }

  http.end();
}

#else
  // Stubs when WiFi is disabled — keeps main code clean
  void connectWiFi()       {}
  void sendTelegramAlert() {}
#endif // ENABLE_WIFI_ALERT

#endif // WIFI_ALERT_H
