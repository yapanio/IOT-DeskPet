/**
 * @file sensors.h
 * @brief Quản lý đọc dữ liệu từ các CẢM BIẾN môi trường.
 *
 * File này chứa class Sensors chịu trách nhiệm:
 *  - Đọc nhiệt độ và độ ẩm từ cảm biến DHT11
 *  - Đọc cường độ ánh sáng từ cảm biến BH1750
 *  - Tính toán "chỉ số cảm giác nóng" (Heat Index)
 *  - Hỗ trợ chế độ giả lập (Mock Mode) để test mà không cần phần cứng thật
 *
 * PHẦN CỨNG SỬ DỤNG:
 *  - DHT11: Cảm biến nhiệt độ & độ ẩm (giao tiếp 1-Wire)
 *  - BH1750: Cảm biến ánh sáng (giao tiếp I2C - địa chỉ 0x23)
 */

#ifndef SENSORS_H
#define SENSORS_H

#include <BH1750.h>  // Thư viện cảm biến ánh sáng BH1750
#include <DHT.h>     // Thư viện cảm biến DHT11
#include <Wire.h>    // Thư viện giao tiếp I2C (dùng cho BH1750 và OLED)

// Loại cảm biến DHT đang dùng (DHT11 hoặc DHT22)
#define DHTTYPE DHT11

/**
 * @class Sensors
 * @brief Đóng gói tất cả logic đọc cảm biến vào một class gọn gàng.
 *
 * Cách dùng:
 *   Sensors sensors(19);  // Khai báo với chân GPIO của DHT11
 *   sensors.begin();      // Khởi động cảm biến
 *   sensors.update();     // Gọi liên tục trong loop() để cập nhật số liệu
 *   float t = sensors.getTemperature();  // Lấy nhiệt độ
 */
class Sensors {
 private:
  // ===================================================
  // PHẦN CỨNG
  // ===================================================
  DHT dht;          // Đối tượng cảm biến DHT11
  BH1750 lightMeter; // Đối tượng cảm biến ánh sáng BH1750

  // ===================================================
  // DỮ LIỆU CẢM BIẾN (được cập nhật định kỳ)
  // ===================================================
  float temperature = 0.0;  // Nhiệt độ hiện tại (°C)
  float humidity    = 0.0;  // Độ ẩm hiện tại (%)
  float heatIndex   = 0.0;  // Chỉ số cảm giác nóng - "feels like" (°C)
  float lightLux    = 0.0;  // Cường độ ánh sáng (lux)

  // ===================================================
  // BỘ ĐẾM THỜI GIAN (giúp đọc cảm biến không đồng bộ)
  // ===================================================
  // "Không đồng bộ" (non-blocking) nghĩa là: thay vì dừng lại chờ
  // cảm biến trả kết quả, ta chỉ đọc khi đã đến lúc, rồi tiếp tục
  // làm việc khác ngay. Điều này giúp robot luôn phản hồi nhanh.
  unsigned long lastDhtRead   = 0;  // Thời điểm đọc DHT11 lần cuối (ms)
  unsigned long lastLightRead = 0;  // Thời điểm đọc BH1750 lần cuối (ms)

  // Khoảng cách thời gian giữa các lần đọc
  const unsigned long DHT_READ_INTERVAL   = 2000; // Đọc DHT11 mỗi 2 giây
  const unsigned long LIGHT_READ_INTERVAL = 1000; // Đọc BH1750 mỗi 1 giây

  // ===================================================
  // CHẾ ĐỘ GIẢ LẬP (Mock Mode) - Dùng để TEST
  // ===================================================
  // Khi mockMode = true, thay vì đọc cảm biến thật,
  // chương trình sẽ dùng các giá trị giả lập bên dưới.
  // Rất hữu ích để kiểm tra các trạng thái cảnh báo mà
  // không cần tạo ra điều kiện môi trường thật (như đốt lửa!)
  bool  mockMode  = false; // Có đang ở chế độ giả lập không?
  float mockTemp  = 25.0;  // Nhiệt độ giả lập (°C)
  float mockHumid = 55.0;  // Độ ẩm giả lập (%)
  float mockLux   = 350.0; // Ánh sáng giả lập (lux)

 public:
  /**
   * @brief Hàm khởi tạo - chỉ nhận chân GPIO của DHT11.
   * @param dhtPin Số chân GPIO kết nối với dây DATA của DHT11
   */
  Sensors(int dhtPin) : dht(dhtPin, DHTTYPE) {}

  /**
   * @brief Khởi động tất cả cảm biến. Gọi một lần trong setup().
   */
  void begin() {
    // Khởi động DHT11
    dht.begin();

    // Khởi động I2C và BH1750
    Wire.begin();
    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23)) {
      Serial.println(F("[Sensors] BH1750 (Cam bien anh sang) khoi dong OK."));
    } else {
      Serial.println(F("[Sensors] LOI: BH1750 khoi dong that bai!"));
    }
  }

  /**
   * @brief Bật/tắt chế độ giả lập và thiết lập giá trị giả.
   *
   * Ví dụ giả lập cháy: sensors.setMock(true, 45.0, 50.0, 350.0);
   * Tắt giả lập:        sensors.setMock(false, 0, 0, 0);
   *
   * @param enable Bật (true) hay tắt (false) chế độ giả lập
   * @param temp   Nhiệt độ giả (°C)
   * @param humid  Độ ẩm giả (%)
   * @param lux    Ánh sáng giả (lux)
   */
  void setMock(bool enable, float temp, float humid, float lux) {
    mockMode  = enable;
    mockTemp  = temp;
    mockHumid = humid;
    mockLux   = lux;
  }

  /** @brief Kiểm tra xem chế độ giả lập có đang bật không. */
  bool isMockEnabled() const { return mockMode; }

  /**
   * @brief Cập nhật số liệu cảm biến. Gọi liên tục trong loop().
   *
   * Hàm này sẽ đọc lại cảm biến chỉ khi đã đến lúc (theo interval).
   * Nếu đang ở chế độ mock, sẽ dùng giá trị giả thay vì đọc thật.
   */
  void update() {
    // --- Chế độ giả lập: dùng giá trị mock, bỏ qua đọc phần cứng ---
    if (mockMode) {
      temperature = mockTemp;
      humidity    = mockHumid;
      // Tính heat index từ nhiệt độ và độ ẩm giả lập
      heatIndex   = dht.computeHeatIndex(temperature, humidity, false);
      lightLux    = mockLux;
      return;
    }

    // --- Chế độ thật: đọc từ phần cứng ---
    unsigned long now = millis(); // Lấy thời gian hiện tại (ms kể từ khi bật)

    // Đọc DHT11 nếu đã đến thời điểm (mỗi 2 giây)
    if (now - lastDhtRead >= DHT_READ_INTERVAL || lastDhtRead == 0) {
      float tempRead  = dht.readTemperature(); // Đọc nhiệt độ
      float humidRead = dht.readHumidity();    // Đọc độ ẩm

      // Kiểm tra dữ liệu có hợp lệ không (isnan = "is not a number")
      if (!isnan(tempRead) && !isnan(humidRead)) {
        temperature = tempRead;
        humidity    = humidRead;
        heatIndex   = dht.computeHeatIndex(temperature, humidity, false);
      } else {
        Serial.println(F("[Sensors] LOI: Doc DHT11 that bai!"));
      }
      lastDhtRead = now; // Lưu lại thời điểm đọc lần này
    }

    // Đọc BH1750 nếu đã đến thời điểm (mỗi 1 giây)
    if (now - lastLightRead >= LIGHT_READ_INTERVAL || lastLightRead == 0) {
      float luxRead = lightMeter.readLightLevel(); // Đọc ánh sáng
      if (luxRead >= 0) {
        lightLux = luxRead;
      } else {
        Serial.println(F("[Sensors] LOI: Doc BH1750 that bai!"));
      }
      lastLightRead = now;
    }
  }

  // ===================================================
  // CÁC HÀM LẤY DỮ LIỆU (Getters)
  // ===================================================
  /** @return Nhiệt độ hiện tại (°C) */
  float getTemperature() const { return temperature; }

  /** @return Độ ẩm hiện tại (%) */
  float getHumidity() const { return humidity; }

  /** @return Chỉ số cảm giác nóng - Heat Index (°C) */
  float getHeatIndex() const { return heatIndex; }

  /** @return Cường độ ánh sáng (lux) */
  float getLightLux() const { return lightLux; }
};

#endif  // SENSORS_H
