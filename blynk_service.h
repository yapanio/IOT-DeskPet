/**
 * @file blynk_service.h
 * @brief Chiếc bưu điện thần kỳ - Gửi thông tin của Robot lên điện thoại qua mạng Internet (WiFi)!
 *
 * Blynk giống như một chiếc hộp thư ma thuật. Khi cắm điện và kết nối WiFi, robot sẽ tự động
 * viết thư báo cáo tình hình thời tiết trong phòng (Nhiệt độ, Độ ẩm, Ánh sáng) và gửi bay lên đám mây (Internet).
 * Sau đó, bố mẹ hay con chỉ cần mở điện thoại ra là có thể xem được robot đang cảm thấy thế nào!
 *
 * CÁC HỘP THƯ ẢO (Virtual Pins):
 *  - V0: Nhiệt độ trong phòng (°C)
 *  - V1: Độ ẩm (%)
 *  - V2: Độ sáng phòng (lux)
 *  - V3: Cảm giác nóng thực tế (°C)
 *  - V4: Tâm trạng hiện tại của robot (Ví dụ: "VUI VẺ", "BUỒN NGỦ")
 *  - V5: Nhận lệnh phát nhạc từ điện thoại gửi xuống robot
 *  - V6: Nhận lệnh chạm ảo từ điện thoại gửi xuống robot
 */

#ifndef BLYNK_SERVICE_H
#define BLYNK_SERVICE_H

// Chìa khóa bí mật để robot gửi thư đúng vào tài khoản của con trên mạng
#define BLYNK_TEMPLATE_ID   "TMPL6pSADmMmU"
#define BLYNK_TEMPLATE_NAME "Virtual Pet"
#define BLYNK_AUTH_TOKEN    "mGSykjx_jp097tJWVFMSCtemE5mOYp7B"

// Cho phép robot in thông tin gửi thư ra máy tính để debug
#define BLYNK_PRINT Serial

#include <WiFi.h>              // Thư viện giúp mạch bắt sóng WiFi
#include <WiFiClient.h>        // Thư viện kết nối mạng nội bộ
#include <BlynkSimpleEsp32.h>  // Thư viện Blynk chuyên dành cho mạch ESP32

// ===================================================
// TÊN VÀ MẬT KHẨU WIFI CỦA NHÀ CON
// ===================================================
#define WIFI_SSID "luwukien"      // Tên mạng WiFi nhà con
#define WIFI_PASS "aloalo1234"    // Mật khẩu để kết nối WiFi

/**
 * @class BlynkService
 * @brief Quản lý bưu điện WiFi và gửi nhận dữ liệu với điện thoại.
 *
 * Chức năng tự cứu hộ (Fault-tolerant):
 *  - Nếu nhà con mất mạng lúc bật robot -> Robot vẫn chơi bình thường tại nhà (chế độ OFFLINE), chỉ là không gửi được thư đi thôi.
 *  - Khi nào nhà con có mạng lại -> Robot sẽ tự động kết nối lại và gửi thư tiếp!
 */
class BlynkService {
 private:
  unsigned long lastBlynkUpdate = 0; // Lần cuối gửi thư là lúc nào? (ms)

  // Cứ sau 5 giây (5000 mili-giây) robot lại gửi thư báo cáo thời tiết 1 lần
  const unsigned long BLYNK_UPDATE_INTERVAL = 5000;

  bool wifiConnected = false; // Hỏi xem robot đã bắt được sóng WiFi chưa?

 public:
  BlynkService() {}

  /**
   * @brief Bắt đầu kết nối WiFi và chuẩn bị hộp thư Blynk. Gọi một lần lúc khởi động.
   *
   * Robot sẽ cố gắng bắt sóng WiFi trong 10 giây.
   */
  void begin() {
    Serial.println(F("[WiFi] Đang bắt sóng WiFi..."));
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    // Chờ tối đa 10 giây (10000ms) để xem WiFi có kết nối được không
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println(F("\n[WiFi] Yeah! Đã kết nối WiFi thành công rồi!"));
      wifiConnected = true;
      Blynk.config(BLYNK_AUTH_TOKEN); // Chuẩn bị chìa khóa hòm thư
      Blynk.connect();                // Bắt đầu gửi thư ảo
    } else {
      Serial.println(F("\n[WiFi] Không bắt được WiFi! Robot sẽ chạy chế độ KHÔNG MẠNG (OFFLINE)."));
      wifiConnected = false;
    }
  }

  /**
   * @brief Gửi số liệu lên điện thoại. Hàm này chạy liên tục trong loop().
   */
  void update(float temp, float humid, float feel, float lux, const char* state) {

    // Nếu WiFi vẫn đang chạy tốt
    if (WiFi.status() == WL_CONNECTED) {

      // Nếu trước đó mất mạng mà bây giờ tự dưng có lại
      if (!wifiConnected) {
        Serial.println(F("[WiFi] Có mạng lại rồi! Đang kết nối lại Blynk..."));
        wifiConnected = true;
        Blynk.config(BLYNK_AUTH_TOKEN);
        Blynk.connect();
      }

      // Xử lý các lá thư điện thoại gửi xuống cho robot (ví dụ lệnh bắt robot nhảy múa)
      Blynk.run();

      // Cứ sau 5 giây thì viết thư gửi lên đám mây 1 lần
      unsigned long now = millis();
      if (now - lastBlynkUpdate >= BLYNK_UPDATE_INTERVAL || lastBlynkUpdate == 0) {
        lastBlynkUpdate = now;

        if (Blynk.connected()) {
          // Viết số liệu đo đạc vào các ô thư ảo
          Blynk.virtualWrite(V0, temp);   // Ghi Nhiệt độ
          Blynk.virtualWrite(V1, humid);  // Ghi Độ ẩm
          Blynk.virtualWrite(V2, lux);    // Ghi Ánh sáng
          Blynk.virtualWrite(V3, feel);   // Ghi Cảm giác nóng
          Blynk.virtualWrite(V4, state);  // Ghi Tâm trạng hiện tại
          Serial.println(F("[Blynk] Đã gửi thư báo cáo thời tiết thành công!"));
        } else {
          Serial.println(F("[Blynk] Đang nghẽn mạng Blynk, chờ thử lại..."));
        }
      }

    } else {
      // Nếu tự dưng mất WiFi giữa chừng
      if (wifiConnected) {
        Serial.println(F("[WiFi] Ôi! Mất sóng WiFi rồi!"));
        wifiConnected = false;
      }
    }
  }
};

#endif  // BLYNK_SERVICE_H
