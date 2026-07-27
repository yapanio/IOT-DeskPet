#ifndef BLYNK_SERVICE_H
#define BLYNK_SERVICE_H

#define BLYNK_TEMPLATE_ID   "TMPL6pSADmMmU"
#define BLYNK_TEMPLATE_NAME "Virtual Pet"
#define BLYNK_AUTH_TOKEN    "mGSykjx_jp097tJWVFMSCtemE5mOYp7B"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

#define WIFI_SSID "luwukien"
#define WIFI_PASS "aloalo1234"

class BlynkService {
 private:
  unsigned long lastBlynkUpdate = 0;

  const unsigned long BLYNK_UPDATE_INTERVAL = 5000;

  bool wifiConnected = false;

 public:
  BlynkService() {}

  void begin() {
    Serial.println(F("[WiFi] Đang bắt sóng WiFi..."));
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println(F("\n[WiFi] Yeah! Đã kết nối WiFi thành công rồi!"));
      wifiConnected = true;
      Blynk.config(BLYNK_AUTH_TOKEN);
      Blynk.connect();
    } else {
      Serial.println(F("\n[WiFi] Không bắt được WiFi! Robot sẽ chạy chế độ KHÔNG MẠNG (OFFLINE)."));
      wifiConnected = false;
    }
  }

  void update(float temp, float humid, float feel, float lux, const char* state) {

    if (WiFi.status() == WL_CONNECTED) {

      if (!wifiConnected) {
        Serial.println(F("[WiFi] Có mạng lại rồi! Đang kết nối lại Blynk..."));
        wifiConnected = true;
        Blynk.config(BLYNK_AUTH_TOKEN);
        Blynk.connect();
      }

      Blynk.run();

      unsigned long now = millis();
      if (now - lastBlynkUpdate >= BLYNK_UPDATE_INTERVAL || lastBlynkUpdate == 0) {
        lastBlynkUpdate = now;

        if (Blynk.connected()) {
          Blynk.virtualWrite(V0, temp);
          Blynk.virtualWrite(V1, humid);
          Blynk.virtualWrite(V2, lux);
          Blynk.virtualWrite(V3, feel);
          Blynk.virtualWrite(V4, state);
          Serial.println(F("[Blynk] Đã gửi thư báo cáo thời tiết thành công!"));
        } else {
          Serial.println(F("[Blynk] Đang nghẽn mạng Blynk, chờ thử lại..."));
        }
      }

    } else {
      if (wifiConnected) {
        Serial.println(F("[WiFi] Ôi! Mất sóng WiFi rồi!"));
        wifiConnected = false;
      }
    }
  }
};

#endif
