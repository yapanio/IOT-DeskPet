#ifndef BLYNK_SERVICE_H
#define BLYNK_SERVICE_H

#define BLYNK_TEMPLATE_ID "TMPL6pSADmMmU"
#define BLYNK_TEMPLATE_NAME "Virtual Pet"
#define BLYNK_AUTH_TOKEN "mGSykjx_jp097tJWVFMSCtemE5mOYp7B"

#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// WiFi Configuration - Thay đổi thông tin Wifi của bạn ở đây
#define WIFI_SSID "luwukien"
#define WIFI_PASS "aloalo1234"

class BlynkService {
private:
    unsigned long lastBlynkUpdate = 0;
    const unsigned long blynkInterval = 5000; // Gửi dữ liệu lên Blynk mỗi 5 giây
    bool wifiConnected = false;

public:
    BlynkService() {}

    void begin() {
        Serial.println(F("Connecting to WiFi..."));
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        
        // Chờ kết nối WiFi tối đa 10 giây (non-blocking boot)
        unsigned long startAttemptTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
            delay(500);
            Serial.print(".");
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println(F("\nWiFi connected successfully!"));
            wifiConnected = true;
            Blynk.config(BLYNK_AUTH_TOKEN);
            Blynk.connect(); // Kết nối Blynk không đồng bộ (asynchronous)
        } else {
            Serial.println(F("\nWiFi connection failed! Starting in OFFLINE mode."));
            wifiConnected = false;
        }
    }

    void update(float temp, float humid, float feel, float lux, const char* state) {
        // Nếu kết nối WiFi thành công, chạy Blynk
        if (WiFi.status() == WL_CONNECTED) {
            if (!wifiConnected) {
                // Tự động kết nối lại nếu trước đó bị mất kết nối hoặc khởi động offline
                wifiConnected = true;
                Blynk.config(BLYNK_AUTH_TOKEN);
                Blynk.connect();
            }
            
            Blynk.run();

            unsigned long now = millis();
            if (now - lastBlynkUpdate >= blynkInterval || lastBlynkUpdate == 0) {
                lastBlynkUpdate = now;
                if (Blynk.connected()) {
                    Blynk.virtualWrite(V0, temp);
                    Blynk.virtualWrite(V1, humid);
                    Blynk.virtualWrite(V2, lux);
                    Blynk.virtualWrite(V3, feel);
                    Blynk.virtualWrite(V4, state);
                    Serial.println(F("[Blynk] Data pushed to cloud successfully."));
                } else {
                    Serial.println(F("[Blynk] Blynk server disconnected, attempting reconnect..."));
                }
            }
        } else {
            if (wifiConnected) {
                Serial.println(F("[Blynk] WiFi connection lost!"));
                wifiConnected = false;
            }
        }
    }
};

#endif
