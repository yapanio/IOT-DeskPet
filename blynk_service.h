/**
 * @file blynk_service.h
 * @brief Kết nối robot với ứng dụng BLYNK qua WiFi (IoT Cloud).
 *
 * Blynk là nền tảng IoT cho phép điều khiển thiết bị qua điện thoại.
 * File này chứa class BlynkService chịu trách nhiệm:
 *  - Kết nối WiFi và Blynk khi khởi động
 *  - Tự động kết nối lại khi mất kết nối
 *  - Gửi số liệu cảm biến lên Blynk mỗi 5 giây
 *  - Hỗ trợ chế độ OFFLINE nếu không có WiFi
 *
 * VIRTUAL PINS (Chân ảo trên Blynk):
 *  V0 -> Nhiệt độ (°C)
 *  V1 -> Độ ẩm (%)
 *  V2 -> Ánh sáng (lux)
 *  V3 -> Heat Index (°C)
 *  V4 -> Trạng thái robot (text)
 *  V5 -> Nhận lệnh phát nhạc từ ứng dụng (1=Mario, 2=Despacito, 3=Jingle Bells, 0=Dừng)
 *  V6 -> Nhận lệnh chạm ảo từ ứng dụng (1=Nhấn, 0=Thả)
 */

#ifndef BLYNK_SERVICE_H
#define BLYNK_SERVICE_H

// Thông tin xác thực Blynk - LẤY TỪ BLYNK CONSOLE
#define BLYNK_TEMPLATE_ID   "TMPL6pSADmMmU"
#define BLYNK_TEMPLATE_NAME "Virtual Pet"
#define BLYNK_AUTH_TOKEN    "mGSykjx_jp097tJWVFMSCtemE5mOYp7B"

// Cho phép Blynk in log ra Serial Monitor
#define BLYNK_PRINT Serial

#include <WiFi.h>              // Thư viện WiFi của ESP32
#include <WiFiClient.h>        // Kết nối WiFi phía client
#include <BlynkSimpleEsp32.h>  // Thư viện Blynk cho ESP32

// ===================================================
// THÔNG TIN WIFI - THAY ĐỔI Ở ĐÂY!
// ===================================================
#define WIFI_SSID "luwukien"      // Tên mạng WiFi (SSID)
#define WIFI_PASS "aloalo1234"    // Mật khẩu WiFi

/**
 * @class BlynkService
 * @brief Quản lý toàn bộ kết nối WiFi và giao tiếp với Blynk IoT Cloud.
 *
 * Thiết kế chịu lỗi (fault-tolerant):
 *  - Nếu không kết nối được WiFi lúc khởi động -> chạy OFFLINE bình thường
 *  - Nếu WiFi xuất hiện sau khi đã chạy OFFLINE -> tự động kết nối lại
 *  - Nếu mất kết nối giữa chừng -> tự phát hiện và thử kết nối lại
 */
class BlynkService {
 private:
  // Thời điểm gửi dữ liệu lên Blynk lần cuối
  unsigned long lastBlynkUpdate = 0;

  // Khoảng cách giữa các lần gửi dữ liệu (ms)
  const unsigned long BLYNK_UPDATE_INTERVAL = 5000; // 5 giây 1 lần

  // Theo dõi trạng thái WiFi để phát hiện kết nối/mất kết nối
  bool wifiConnected = false;

 public:
  BlynkService() {}

  /**
   * @brief Kết nối WiFi và Blynk. Gọi một lần trong setup().
   *
   * Thử kết nối WiFi tối đa 10 giây. Nếu không được -> OFFLINE mode.
   * OFFLINE mode: robot hoạt động đầy đủ, chỉ không gửi dữ liệu lên cloud.
   */
  void begin() {
    Serial.println(F("[WiFi] Dang ket noi WiFi..."));
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    // Chờ kết nối, tối đa 10 giây (10000ms)
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      // Kết nối WiFi thành công
      Serial.println(F("\n[WiFi] Ket noi WiFi thanh cong!"));
      wifiConnected = true;
      Blynk.config(BLYNK_AUTH_TOKEN); // Cấu hình Blynk với auth token
      Blynk.connect();                // Kết nối Blynk (không đồng bộ)
    } else {
      // Không kết nối được -> chạy OFFLINE
      Serial.println(F("\n[WiFi] Ket noi WiFi that bai! Chay che do OFFLINE."));
      wifiConnected = false;
    }
  }

  /**
   * @brief Cập nhật Blynk và gửi số liệu. Gọi liên tục trong loop().
   *
   * Hàm này:
   *  1. Kiểm tra xem WiFi có đang kết nối không
   *  2. Nếu WiFi trở lại sau OFFLINE -> tự kết nối lại Blynk
   *  3. Gọi Blynk.run() để xử lý các lệnh nhận từ ứng dụng
   *  4. Gửi số liệu cảm biến lên Blynk mỗi 5 giây
   *
   * @param temp  Nhiệt độ (°C)
   * @param humid Độ ẩm (%)
   * @param feel  Heat Index - Cảm giác nóng (°C)
   * @param lux   Ánh sáng (lux)
   * @param state Tên trạng thái robot (text, ví dụ: "NORMAL_HAPPY")
   */
  void update(float temp, float humid, float feel, float lux, const char* state) {

    if (WiFi.status() == WL_CONNECTED) {
      // WiFi đang kết nối

      if (!wifiConnected) {
        // WiFi vừa kết nối lại sau khi bị mất -> Kết nối lại Blynk
        Serial.println(F("[WiFi] WiFi ket noi lai! Dang ket noi Blynk..."));
        wifiConnected = true;
        Blynk.config(BLYNK_AUTH_TOKEN);
        Blynk.connect();
      }

      // Xử lý các gói tin từ Blynk (callbacks từ ứng dụng điện thoại)
      Blynk.run();

      // Gửi dữ liệu lên Blynk theo định kỳ (mỗi 5 giây)
      unsigned long now = millis();
      if (now - lastBlynkUpdate >= BLYNK_UPDATE_INTERVAL || lastBlynkUpdate == 0) {
        lastBlynkUpdate = now;

        if (Blynk.connected()) {
          // Gửi số liệu lên các Virtual Pin tương ứng
          Blynk.virtualWrite(V0, temp);   // Nhiệt độ
          Blynk.virtualWrite(V1, humid);  // Độ ẩm
          Blynk.virtualWrite(V2, lux);    // Ánh sáng
          Blynk.virtualWrite(V3, feel);   // Heat Index
          Blynk.virtualWrite(V4, state);  // Trạng thái robot
          Serial.println(F("[Blynk] Da gui du lieu len cloud."));
        } else {
          Serial.println(F("[Blynk] Mat ket noi Blynk, dang thu lai..."));
        }
      }

    } else {
      // WiFi bị mất kết nối
      if (wifiConnected) {
        Serial.println(F("[WiFi] Mat ket noi WiFi!"));
        wifiConnected = false;
      }
      // Không làm gì thêm -> robot tiếp tục hoạt động bình thường (OFFLINE)
    }
  }
};

#endif  // BLYNK_SERVICE_H
