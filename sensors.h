/**
 * @file sensors.h
 * @brief Giác quan của Robot - Giúp Robot cảm nhận môi trường xung quanh!
 *
 * Để biết được trời nóng hay lạnh, sáng hay tối, chú robot của chúng ta cần có "giác quan":
 *  - Cảm biến nhiệt độ & độ ẩm DHT11 giống như làn da của robot.
 *  - Cảm biến ánh sáng BH1750 giống như đôi mắt của robot nhìn xem phòng có sáng không.
 *
 * Tất cả số liệu đo được sẽ được gửi về "bộ não" điều khiển.
 */

#ifndef SENSORS_H
#define SENSORS_H

#include <BH1750.h>  // Thư viện cảm biến ánh sáng BH1750
#include <DHT.h>     // Thư viện cảm biến nhiệt độ DHT11
#include <Wire.h>    // Dùng đường truyền I2C để nói chuyện với BH1750 và màn hình OLED

// Loại cảm biến DHT là DHT11 (như một mẫu da cơ bản)
#define DHTTYPE DHT11

/**
 * @class Sensors
 * @brief Hộp chứa tất cả các giác quan của Robot.
 */
class Sensors {
 private:
  // ===================================================
  // CÁC THIẾT BỊ PHẦN CỨNG (Giác quan vật lý)
  // ===================================================
  DHT dht;           // Cảm biến da DHT11 để đo nóng lạnh
  BH1750 lightMeter; // Cảm biến mắt BH1750 để đo ánh sáng

  // ===================================================
  // THÔNG TIN ĐO ĐƯỢC (Được cập nhật thường xuyên)
  // ===================================================
  float temperature = 0.0;  // Nhiệt độ hiện tại (°C)
  float humidity    = 0.0;  // Độ ẩm hiện tại (%)
  float heatIndex   = 0.0;  // Cảm giác nóng thực tế (°C)
  float lightLux    = 0.0;  // Độ sáng của phòng (đơn vị lux)

  // ===================================================
  // ĐỒNG HỒ ĐẾM THỜI GIAN (Hẹn giờ không làm phiền robot)
  // ===================================================
  // Robot không dùng lệnh delay() (ngủ đông), mà dùng cách "nhìn đồng hồ".
  // Mỗi khi muốn đọc thông số, robot sẽ xem đã đến giờ hẹn chưa.
  // Nhờ thế robot vẫn lắc đầu và hát nhạc mượt mà.
  unsigned long lastDhtRead   = 0;  // Lần cuối cùng đo nhiệt độ là lúc nào? (ms)
  unsigned long lastLightRead = 0;  // Lần cuối cùng đo ánh sáng là lúc nào? (ms)

  // Khoảng thời gian hẹn trước giữa mỗi lần đo
  const unsigned long DHT_READ_INTERVAL   = 2000; // Cứ mỗi 2 giây đo nhiệt độ 1 lần
  const unsigned long LIGHT_READ_INTERVAL = 1000; // Cứ mỗi 1 giây đo ánh sáng 1 lần

  // ===================================================
  // CHẾ ĐỘ ĐÓNG KỊCH (Giả lập - Mock Mode để Test)
  // ===================================================
  // Khi bật chế độ này lên (mockMode = true), robot sẽ "giả vờ" như trời
  // đang cực kỳ nóng hoặc lạnh để chúng ta kiểm tra xem robot có biết báo động không.
  // Nhờ thế chúng ta không cần dùng lửa thật hay nước đá thật để thử robot!
  bool  mockMode  = false; // Có đang giả vờ hay không?
  float mockTemp  = 25.0;  // Nhiệt độ giả vờ (°C)
  float mockHumid = 55.0;  // Độ ẩm giả vờ (%)
  float mockLux   = 350.0; // Ánh sáng giả vờ (lux)

 public:
  /**
   * @brief Hàm chuẩn bị chân cắm cho cảm biến nhiệt độ.
   * @param dhtPin Số cổng cắm dây dữ liệu của DHT11 trên ESP32
   */
  Sensors(int dhtPin) : dht(dhtPin, DHTTYPE) {}

  /**
   * @brief Khởi động các giác quan của robot.
   */
  void begin() {
    // Đánh thức cảm biến nhiệt độ dậy
    dht.begin();

    // Bắt đầu đường nói chuyện I2C và khởi động cảm biến ánh sáng
    Wire.begin();
    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23)) {
      Serial.println(F("[Cảm biến] Mắt đo ánh sáng BH1750 đã sẵn sàng!"));
    } else {
      Serial.println(F("[Cảm biến] Ối! Không tìm thấy mắt đo ánh sáng BH1750 rồi!"));
    }
  }

  /**
   * @brief Ra lệnh cho robot chuyển sang chế độ "đóng kịch" (giả lập).
   *
   * Ví dụ đóng kịch có cháy: setMock(true, 45.0, 50.0, 350.0);
   * Thoát đóng kịch:        setMock(false, 0, 0, 0);
   */
  void setMock(bool enable, float temp, float humid, float lux) {
    mockMode  = enable;
    mockTemp  = temp;
    mockHumid = humid;
    mockLux   = lux;
  }

  /** @brief Hỏi xem robot có đang đóng kịch giả lập hay không. */
  bool isMockEnabled() const { return mockMode; }

  /**
   * @brief Cập nhật thông số từ môi trường. Hàm này chạy liên tục trong loop().
   */
  void update() {
    // --- Nếu đang đóng kịch: Nhận luôn thông số giả vờ, không đo thật nữa ---
    if (mockMode) {
      temperature = mockTemp;
      humidity    = mockHumid;
      heatIndex   = dht.computeHeatIndex(temperature, humidity, false);
      lightLux    = mockLux;
      return;
    }

    // --- Nếu đang đo thật: Nhìn đồng hồ xem đã đến lúc đọc cảm biến chưa ---
    unsigned long now = millis(); // Thời gian tính bằng mili-giây kể từ khi cắm điện

    // Cứ sau 2 giây đo da nhiệt độ một lần
    if (now - lastDhtRead >= DHT_READ_INTERVAL || lastDhtRead == 0) {
      float tempRead  = dht.readTemperature(); // Đọc nhiệt độ thật
      float humidRead = dht.readHumidity();    // Đọc độ ẩm thật

      // Kiểm tra xem số liệu đọc về có bị lỗi không (isnan = không phải số hợp lệ)
      if (!isnan(tempRead) && !isnan(humidRead)) {
        temperature = tempRead;
        humidity    = humidRead;
        heatIndex   = dht.computeHeatIndex(temperature, humidity, false);
      } else {
        Serial.println(F("[Cảm biến] Cảnh báo: Lỗi không đọc được cảm biến DHT11 rồi!"));
      }
      lastDhtRead = now; // Lưu lại thời gian vừa đọc xong
    }

    // Cứ sau 1 giây đo mắt ánh sáng một lần
    if (now - lastLightRead >= LIGHT_READ_INTERVAL || lastLightRead == 0) {
      float luxRead = lightMeter.readLightLevel(); // Đọc độ sáng thật
      if (luxRead >= 0) {
        lightLux = luxRead;
      } else {
        Serial.println(F("[Cảm biến] Cảnh báo: Lỗi không đọc được mắt ánh sáng BH1750!"));
      }
      lastLightRead = now; // Lưu lại thời gian vừa đọc xong
    }
  }

  // ===================================================
  // CÁC NÚT BẤM LẤY SỐ LIỆU ĐỂ HỎI THÔNG TIN (Getters)
  // ===================================================
  /** @return Nhiệt độ phòng hiện tại (°C) */
  float getTemperature() const { return temperature; }

  /** @return Độ ẩm phòng hiện tại (%) */
  float getHumidity() const { return humidity; }

  /** @return Cảm giác nóng thực tế (°C) */
  float getHeatIndex() const { return heatIndex; }

  /** @return Cường độ ánh sáng (lux) */
  float getLightLux() const { return lightLux; }
};

#endif  // SENSORS_H
