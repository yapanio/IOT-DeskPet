/**
 * @file emote.h
 * @brief Khuôn mặt của Robot - Màn hình OLED và đôi mắt hoạt hình biết nói!
 *
 * Màn hình OLED SSD1306 rộng 128 điểm ảnh và cao 64 điểm ảnh giống như một "khuôn mặt gương" của robot.
 * Chúng ta sử dụng thư viện RoboEyes để vẽ hai bầu mắt hoạt hình cực kỳ sinh động:
 *  - Mắt biết chớp tự động.
 *  - Mắt biết liếc nhìn xung quanh ngẫu nhiên khi vui vẻ.
 *  - Mắt nhắm tịt buồn ngủ khi tắt đèn.
 *  - Mắt nháy mắt (Wink) trêu đùa khi được chạm 2 lần.
 *  - Mặt vẽ hình X_X khi phòng quá nóng có nguy cơ cháy!
 */

#ifndef EMOTE_H
#define EMOTE_H

#include "robot_state.h"
#include "sensors.h"
#include <Adafruit_GFX.h>       // Thư viện đồ họa cơ bản (đường thẳng, hình tròn...)
#include <Adafruit_SSD1306.h>   // Thư viện điều khiển màn hình OLED SSD1306
#include <FluxGarage_RoboEyes.h> // Thư viện tạo đôi mắt robot hoạt hình

// Kích thước màn hình OLED (pixel)
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64

// Chân RESET của OLED (-1 = dùng RESET của ESP32 thay vì chân riêng)
#define OLED_RESET -1

// Địa chỉ I2C của màn hình OLED (0x3C là địa chỉ phổ biến nhất)
#define SCREEN_ADDRESS 0x3C

/**
 * @class Emote
 * @brief Quản lý hiển thị màn hình OLED: mặt robot và thông số môi trường.
 */
class Emote {
 private:
  // ===================================================
  // PHẦN CỨNG VÀ THƯ VIỆN
  // ===================================================
  Adafruit_SSD1306 display;              // Đối tượng màn hình OLED
  RoboEyes<Adafruit_SSD1306> eyes;       // Đối tượng đôi mắt robot
  RobotState currentState = NORMAL_HAPPY; // Trạng thái hiện tại để biết vẽ mặt gì

  // ===================================================
  // CHẾ ĐỘ HIỂN THỊ MÀN HÌNH
  // ===================================================
  bool showParamScreen = false; // true = Hiển thị thông số, false = Hiển thị mặt

  // ===================================================
  // TRẠNG THÁI NHÌN LIẾC MẮT (Wink)
  // ===================================================
  bool isWinking = false;          // Đang liếc mắt không?
  unsigned long winkEndTime = 0;   // Khi nào kết thúc liếc mắt?

  // Con con trỏ đến đối tượng Sensors để lấy số liệu vẽ lên màn hình
  Sensors* sensors = nullptr;

 public:
  /**
   * @brief Hàm khởi tạo - tạo màn hình và đôi mắt.
   */
  Emote()
      : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET),
        eyes(display) {}

  /**
   * @brief Bật/tắt màn hình hiển thị thông số môi trường.
   */
  void setShowParamScreen(bool show) { showParamScreen = show; }

  /** @return Màn hình thông số có đang hiển thị không? */
  bool getShowParamScreen() const { return showParamScreen; }

  /**
   * @brief Khởi động màn hình OLED và đôi mắt. Gọi một lần trong setup().
   */
  void begin(Sensors* sensorsPtr) {
    sensors = sensorsPtr;

    // Khởi động màn hình OLED
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
      Serial.println(F("[Màn hình] Lỗi: Khởi động màn hình OLED thất bại!"));
      return;
    }
    display.clearDisplay();
    display.display();

    // Khởi động RoboEyes với kích thước màn hình 128x64, tốc độ 30fps
    eyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 30);

    // Cấu hình kích thước và hình dạng mắt mặc định
    eyes.setWidth(30, 30);           // Chiều rộng mỗi mắt 30px
    eyes.setHeight(32, 32);          // Chiều cao mỗi mắt 32px
    eyes.setBorderradius(8, 8);      // Bo tròn góc mắt 8px
    eyes.setSpacebetween(12);        // Khoảng cách giữa 2 mắt 12px
    eyes.setAutoblinker(true, 3, 4); // Tự động chớp mắt ngẫu nhiên

    setExpression(NORMAL_HAPPY); // Bắt đầu với mặt vui vẻ
  }

  /**
   * @brief Thay đổi biểu cảm khuôn mặt theo trạng thái robot.
   */
  void setExpression(RobotState state) {
    currentState = state;

    switch (state) {

      case DANGER_FIRE:
        // Đóng kịch cứu hỏa: Sẽ vẽ mặt chữ X_X trực tiếp trong hàm update()
        eyes.setAutoblinker(false);
        eyes.setIdleMode(false);
        break;

      case DANGER_HUMID:
        // Ướt sũng: mắt mệt mỏi và trán lấm tấm mồ hôi lo lắng!
        eyes.open();
        eyes.setMood(TIRED);
        eyes.setSweat(true);    // Hiệu ứng mồ hôi rơi
        eyes.setHFlicker(false);
        eyes.setAutoblinker(true, 3, 4);
        eyes.setIdleMode(false);
        eyes.setPosition(DEFAULT);
        break;

      case WARNING_HOT:
        // Trời nóng: Mắt lờ đờ mệt mỏi
        eyes.open();
        eyes.setMood(TIRED);
        eyes.setSweat(false);
        eyes.setHFlicker(false);
        eyes.setAutoblinker(true, 3, 4);
        eyes.setIdleMode(false);
        eyes.setPosition(DEFAULT);
        break;

      case WARNING_COLD:
        // Trời lạnh: Mắt run rẩy chao đảo
        eyes.open();
        eyes.setMood(TIRED);
        eyes.setSweat(false);
        eyes.setHFlicker(true, 2); // Rung ngang với cường độ 2 (run rẩy vì lạnh)
        eyes.setAutoblinker(true, 3, 4);
        eyes.setIdleMode(false);
        eyes.setPosition(DEFAULT);
        break;

      case WARNING_DARK:
        // Trời tối lâu: Mắt nheo lại tỏ vẻ nghi ngờ và ngước nhìn lên tìm ánh sáng
        eyes.open();
        eyes.setMood(ANGRY); // ANGRY = mắt nhíu lại
        eyes.setSweat(false);
        eyes.setHFlicker(false);
        eyes.setAutoblinker(true, 3, 4);
        eyes.setIdleMode(false);
        eyes.setPosition(N); // N = North = nhìn lên trên
        break;

      case SLEEP_MODE:
        // Đi ngủ: Nhắm tịt hai mắt lại ngủ khò khò
        eyes.close();
        eyes.setSweat(false);
        eyes.setHFlicker(false);
        eyes.setAutoblinker(false); // Không chớp mắt khi đang ngủ
        eyes.setIdleMode(false);
        eyes.setPosition(DEFAULT);
        break;

      case NORMAL_HAPPY:
        // Vui vẻ: Mắt cười hình cầu vồng và liếc nhìn ngẫu nhiên quanh phòng
        eyes.open();
        eyes.setMood(HAPPY);
        eyes.setSweat(false);
        eyes.setHFlicker(false);
        eyes.setAutoblinker(true, 3, 4);
        eyes.setIdleMode(true, 3, 3); // Tự động nhìn xung quanh
        break;

      case DANCE_MODE:
        // Nhảy múa: Mắt vui vẻ mở to, không chớp mắt tự động để chuẩn bị quay tròn mắt
        eyes.open();
        eyes.setMood(HAPPY);
        eyes.setSweat(false);
        eyes.setHFlicker(false);
        eyes.setAutoblinker(false); // Tắt auto-blink để mắt quay tròn tự nhiên hơn
        eyes.setIdleMode(false);
        eyes.setPosition(DEFAULT);
        break;
    }
  }

  /**
   * @brief Nháy một bên mắt trái trong 1 giây để trêu bé (khi bé chạm 2 lần).
   */
  void triggerWink() {
    isWinking    = true;
    winkEndTime  = millis() + 1000; // Nháy trong 1 giây
    eyes.setAutoblinker(false);
    eyes.close(true, false); // Tham số: (nhắm_trái, nhắm_phải)
  }

  /** @brief Tạo mắt cười tít mắt ngộ nghĩnh */
  void triggerLaugh() { eyes.anim_laugh(); }

  /** @brief Tạo mắt ngơ ngác phân vân */
  void triggerConfused() { eyes.anim_confused(); }

  /**
   * @brief Vẽ màn hình thông số thời tiết chi tiết (nhiệt độ, độ ẩm, ánh sáng).
   * Màn hình được chia làm 3 cột tiến trình dọc ngộ nghĩnh.
   */
  void drawParamScreen() {
    display.clearDisplay();

    // --- Vẽ tiêu đề màn hình ---
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(32, 2);
    display.print(F("ENV STATUS"));
    display.drawFastHLine(0, 12, 128, SSD1306_WHITE); // Đường kẻ ngang dưới header

    // Lấy thông số đo đạc từ cảm biến
    float temp  = (sensors != nullptr) ? sensors->getTemperature() : 0.0;
    float humid = (sensors != nullptr) ? sensors->getHumidity()    : 0.0;
    float light = (sensors != nullptr) ? sensors->getLightLux()    : 0.0;

    // Chia màn hình làm 3 cột bằng 2 vạch kẻ đứng
    display.drawFastVLine(42, 13, 51, SSD1306_WHITE); // Giữa cột 1 và 2
    display.drawFastVLine(85, 13, 51, SSD1306_WHITE); // Giữa cột 2 và 3

    // --- CỘT 1: NHIỆT ĐỘ (TEMP) ---
    display.setCursor(9, 16);
    display.setTextSize(1);
    display.print(F("TEMP"));

    display.setCursor(4, 28);
    display.setTextSize(2); // Số to rõ ràng
    display.print((int)round(temp));
    display.setTextSize(1);
    display.print(F("C"));

    // Vẽ vạch tiến trình: Nhiệt độ từ 0°C đến 50°C ứng với thanh dài tối đa 30 điểm
    int tempBar = map(constrain((int)temp, 0, 50), 0, 50, 0, 30);
    display.drawRect(5, 48, 32, 7, SSD1306_WHITE);  // Viền thanh
    display.fillRect(6, 49, tempBar, 5, SSD1306_WHITE); // Phần ruột trắng

    // --- CỘT 2: ĐỘ ẨM (HUMI) ---
    display.setTextSize(1);
    display.setCursor(52, 16);
    display.print(F("HUMI"));

    display.setCursor(47, 28);
    display.setTextSize(2);
    display.print((int)round(humid));
    display.setTextSize(1);
    display.print(F("%"));

    // Vẽ vạch tiến trình: Độ ẩm từ 0% đến 100% ứng với thanh dài tối đa 30 điểm
    int humidBar = map(constrain((int)humid, 0, 100), 0, 100, 0, 30);
    display.drawRect(48, 48, 32, 7, SSD1306_WHITE);
    display.fillRect(49, 49, humidBar, 5, SSD1306_WHITE);

    // --- CỘT 3: ÁNH SÁNG (LIGHT) ---
    display.setTextSize(1);
    display.setCursor(94, 16);
    display.print(F("LIGHT"));

    int lightVal = (int)round(light);
    display.setCursor(90, 28);
    if (lightVal >= 1000) {
      display.setTextSize(1); // Nếu sáng quá > 1000 lux thì viết chữ nhỏ lại cho đỡ chật màn hình
      display.setCursor(90, 32);
    } else {
      display.setTextSize(2);
    }
    display.print(lightVal);
    display.setTextSize(1);

    // Vẽ vạch tiến trình: Ánh sáng từ 0 đến 1000 lux ứng với thanh dài tối đa 30 điểm
    int lightBar = map(constrain(lightVal, 0, 1000), 0, 1000, 0, 30);
    display.drawRect(91, 48, 32, 7, SSD1306_WHITE);
    display.fillRect(92, 49, lightBar, 5, SSD1306_WHITE);

    display.display(); // Đẩy toàn bộ hình vẽ lên màn hình thật
  }

  /**
   * @brief Cập nhật vẽ mắt hoặc vẽ chữ X_X báo cháy tùy trạng thái. Gọi liên tục trong loop().
   */
  void update() {
    unsigned long now = millis();

    // --- Bước 1: Kết thúc nháy mắt winking nếu đã hết thời gian 1 giây ---
    if (isWinking && now >= winkEndTime) {
      isWinking = false;
      setExpression(currentState); // Khôi phục biểu cảm bình thường
    }

    // --- Bước 2: Hiển thị màn hình thông số nếu đang ở chế độ xem thông số ---
    if (showParamScreen) {
      drawParamScreen();
      return; // Không vẽ mặt nữa
    }

    // --- Bước 3: Vẽ mặt X_X khi có nguy cơ CHÁY (DANGER_FIRE) ---
    if (currentState == DANGER_FIRE) {
      display.clearDisplay();

      // Mắt trái: Hình chữ X (tâm x=43, y=32)
      display.drawLine(31, 20, 55, 44, SSD1306_WHITE); // Đường chéo xuống phải
      display.drawLine(55, 20, 31, 44, SSD1306_WHITE); // Đường chéo xuống trái

      // Mắt phải: Hình chữ X (tâm x=85, y=32)
      display.drawLine(73, 20, 97, 44, SSD1306_WHITE); // Đường chéo xuống phải
      display.drawLine(97, 20, 73, 44, SSD1306_WHITE); // Đường chéo xuống trái

      // Miệng: Dấu gạch ngang _ ở giữa (x=58..70, y=48)
      display.drawLine(58, 48, 70, 48, SSD1306_WHITE);

      display.display();
      return;
    }

    // --- Bước 4: Quay tròn mắt khi nhảy múa (DANCE_MODE) ---
    if (currentState == DANCE_MODE && !isWinking) {
      // Khi nhảy: mắt quay tròn theo 8 hướng, đổi mỗi 200ms
      int directionIndex = (now / 200) % 8;
      int directions[] = {N, NE, E, SE, S, SW, W, NW}; // 8 hướng nhìn
      eyes.setPosition(directions[directionIndex]);
    }

    eyes.update(); // Cập nhật animation của RoboEyes và vẽ lên màn hình
  }
};

#endif  // EMOTE_H
