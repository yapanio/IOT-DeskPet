/**
 * @file emote.h
 * @brief Điều khiển MÀN HÌNH OLED - hiển thị biểu cảm khuôn mặt và thông số môi trường.
 *
 * File này chứa class Emote chịu trách nhiệm:
 *  1. BIỂU CẢM MẶT: Dùng thư viện RoboEyes để vẽ đôi mắt hoạt hình
 *     với các trạng thái cảm xúc khác nhau (vui, mệt, tức giận...).
 *  2. MẶT X_X: Khi phát hiện cháy, vẽ thẳng lên màn hình (bypass RoboEyes).
 *  3. MÀN HÌNH THÔNG SỐ: Hiển thị nhiệt độ, độ ẩm, ánh sáng dạng số và
 *     thanh tiến trình (progress bar) khi người dùng yêu cầu.
 *  4. OVERLAY THÔNG SỐ NHỎ: Dải thông số nhỏ bên phải màn hình khi DANGER_FIRE.
 *
 * PHẦN CỨNG:
 *  - Màn hình OLED SSD1306: 128x64 pixel, giao tiếp I2C, địa chỉ 0x3C
 *  - Thư viện RoboEyes để tạo đôi mắt robot sinh động
 */

#ifndef EMOTE_H
#define EMOTE_H

#include "robot_state.h"
#include "sensors.h"
#include <Adafruit_GFX.h>       // Thư viện đồ họa cơ bản
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

  // Con trỏ đến đối tượng Sensors để lấy số liệu vẽ lên màn hình
  Sensors* sensors = nullptr;

 public:
  /**
   * @brief Hàm khởi tạo - tạo màn hình và đôi mắt.
   * Wire là đối tượng I2C (tạo sẵn trong thư viện Wire.h).
   */
  Emote()
      : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET),
        eyes(display) {}

  /**
   * @brief Bật/tắt màn hình hiển thị thông số môi trường.
   * @param show true = hiện thông số, false = hiện mặt robot
   */
  void setShowParamScreen(bool show) { showParamScreen = show; }

  /** @return Màn hình thông số có đang hiển thị không? */
  bool getShowParamScreen() const { return showParamScreen; }

  /**
   * @brief Khởi động màn hình OLED và đôi mắt. Gọi một lần trong setup().
   * @param sensorsPtr Con trỏ đến đối tượng Sensors (để lấy số liệu hiển thị)
   */
  void begin(Sensors* sensorsPtr) {
    sensors = sensorsPtr;

    // Khởi động màn hình OLED
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
      Serial.println(F("[Emote] LOI: Khoi dong man hinh OLED that bai!"));
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
   * Mỗi trạng thái có một biểu cảm riêng:
   *  - DANGER_FIRE  : Mặt X_X (vẽ thủ công trong update())
   *  - DANGER_HUMID : Mệt mỏi, chảy mồ hôi
   *  - WARNING_HOT  : Mệt mỏi (nóng)
   *  - WARNING_COLD : Mệt mỏi + run rẩy (lạnh)
   *  - WARNING_DARK : Tức giận, nhìn lên (thiếu sáng)
   *  - SLEEP_MODE   : Nhắm mắt (ngủ)
   *  - NORMAL_HAPPY : Vui vẻ, nhìn xung quanh ngẫu nhiên
   *  - DANCE_MODE   : Vui vẻ, mắt quay tròn
   * @param state Trạng thái mới cần hiển thị
   */
  void setExpression(RobotState state) {
    currentState = state;

    switch (state) {

      case DANGER_FIRE:
        // Mặt X_X được vẽ trực tiếp trong update() - RoboEyes tắt
        eyes.setAutoblinker(false);
        eyes.setIdleMode(false);
        break;

      case DANGER_HUMID:
        // Mặt mệt mỏi + chảy mồ hôi (TIRED mood + Sweat = true)
        eyes.open();
        eyes.setMood(TIRED);
        eyes.setSweat(true);    // Hiệu ứng mồ hôi rơi
        eyes.setHFlicker(false);
        eyes.setAutoblinker(true, 3, 4);
        eyes.setIdleMode(false);
        eyes.setPosition(DEFAULT);
        break;

      case WARNING_HOT:
        // Mặt mệt mỏi (nóng quá)
        eyes.open();
        eyes.setMood(TIRED);
        eyes.setSweat(false);
        eyes.setHFlicker(false);
        eyes.setAutoblinker(true, 3, 4);
        eyes.setIdleMode(false);
        eyes.setPosition(DEFAULT);
        break;

      case WARNING_COLD:
        // Mặt mệt mỏi + run rẩy (HFlicker = rung ngang)
        eyes.open();
        eyes.setMood(TIRED);
        eyes.setSweat(false);
        eyes.setHFlicker(true, 2); // Rung ngang với cường độ 2 (run rẩy vì lạnh)
        eyes.setAutoblinker(true, 3, 4);
        eyes.setIdleMode(false);
        eyes.setPosition(DEFAULT);
        break;

      case WARNING_DARK:
        // Mặt tức giận + nhìn lên (đang tìm kiếm ánh sáng)
        eyes.open();
        eyes.setMood(ANGRY); // ANGRY = mắt nhíu lại
        eyes.setSweat(false);
        eyes.setHFlicker(false);
        eyes.setAutoblinker(true, 3, 4);
        eyes.setIdleMode(false);
        eyes.setPosition(N); // N = North = nhìn lên trên
        break;

      case SLEEP_MODE:
        // Nhắm mắt ngủ
        eyes.close();
        eyes.setSweat(false);
        eyes.setHFlicker(false);
        eyes.setAutoblinker(false); // Không chớp mắt khi đang ngủ
        eyes.setIdleMode(false);
        eyes.setPosition(DEFAULT);
        break;

      case NORMAL_HAPPY:
        // Vui vẻ, tự động nhìn xung quanh ngẫu nhiên (IdleMode)
        eyes.open();
        eyes.setMood(HAPPY);
        eyes.setSweat(false);
        eyes.setHFlicker(false);
        eyes.setAutoblinker(true, 3, 4);
        eyes.setIdleMode(true, 3, 3); // Tự động nhìn xung quanh
        break;

      case DANCE_MODE:
        // Vui vẻ khi nhảy múa (mắt quay tròn trong update())
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
   * @brief Kích hoạt hiệu ứng nháy mắt trong 1 giây.
   * Nhắm mắt trái, mở mắt phải.
   */
  void triggerWink() {
    isWinking    = true;
    winkEndTime  = millis() + 1000; // Nháy trong 1 giây
    eyes.setAutoblinker(false);
    eyes.close(true, false); // Tham số: (nhắm_trái, nhắm_phải)
  }

  /** @brief Kích hoạt animation cười (laugh). */
  void triggerLaugh() { eyes.anim_laugh(); }

  /** @brief Kích hoạt animation bối rối (confused). */
  void triggerConfused() { eyes.anim_confused(); }

  /**
   * @brief Vẽ màn hình thông số môi trường đầy đủ.
   *
   * Bố cục màn hình 128x64:
   * ┌────────────────────────────────┐
   * │         ENV STATUS             │  ← Header (y=0..12)
   * ├──────────┬──────────┬──────────┤
   * │   TEMP   │   HUMI   │  LIGHT   │  ← Tiêu đề cột (y=12..26)
   * │   25 C   │  55 %    │   350    │  ← Số liệu lớn (y=26..48)
   * │ [=====.] │ [=====.] │ [=====.] │  ← Thanh tiến trình (y=48..64)
   * └──────────┴──────────┴──────────┘
   */
  void drawParamScreen() {
    display.clearDisplay();

    // --- Header ---
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(32, 2);
    display.print(F("ENV STATUS"));
    display.drawFastHLine(0, 12, 128, SSD1306_WHITE); // Đường kẻ ngang dưới header

    // Lấy số liệu cảm biến (nếu sensors == nullptr thì dùng 0.0)
    float temp  = (sensors != nullptr) ? sensors->getTemperature() : 0.0;
    float humid = (sensors != nullptr) ? sensors->getHumidity()    : 0.0;
    float light = (sensors != nullptr) ? sensors->getLightLux()    : 0.0;

    // Vẽ đường kẻ dọc chia 3 cột
    display.drawFastVLine(42, 13, 51, SSD1306_WHITE); // Giữa cột 1 và 2
    display.drawFastVLine(85, 13, 51, SSD1306_WHITE); // Giữa cột 2 và 3

    // --- Cột 1: Nhiệt độ (TEMP) ---
    display.setCursor(9, 16);
    display.setTextSize(1);
    display.print(F("TEMP"));

    display.setCursor(4, 28);
    display.setTextSize(2); // Số lớn hơn
    display.print((int)round(temp));
    display.setTextSize(1);
    display.print(F("C"));

    // Thanh tiến trình: 0°C = rỗng, 50°C = đầy
    int tempBar = map(constrain((int)temp, 0, 50), 0, 50, 0, 30);
    display.drawRect(5, 48, 32, 7, SSD1306_WHITE);  // Viền thanh
    display.fillRect(6, 49, tempBar, 5, SSD1306_WHITE); // Phần tô

    // --- Cột 2: Độ ẩm (HUMI) ---
    display.setTextSize(1);
    display.setCursor(52, 16);
    display.print(F("HUMI"));

    display.setCursor(47, 28);
    display.setTextSize(2);
    display.print((int)round(humid));
    display.setTextSize(1);
    display.print(F("%"));

    // Thanh tiến trình: 0% = rỗng, 100% = đầy
    int humidBar = map(constrain((int)humid, 0, 100), 0, 100, 0, 30);
    display.drawRect(48, 48, 32, 7, SSD1306_WHITE);
    display.fillRect(49, 49, humidBar, 5, SSD1306_WHITE);

    // --- Cột 3: Ánh sáng (LIGHT) ---
    display.setTextSize(1);
    display.setCursor(94, 16);
    display.print(F("LIGHT"));

    int lightVal = (int)round(light);
    display.setCursor(90, 28);
    // Nếu số >= 1000 (4 chữ số), dùng font nhỏ hơn để vừa cột
    if (lightVal >= 1000) {
      display.setTextSize(1);
      display.setCursor(90, 32);
    } else {
      display.setTextSize(2);
    }
    display.print(lightVal);
    display.setTextSize(1);

    // Thanh tiến trình: 0 lux = rỗng, 1000 lux = đầy
    int lightBar = map(constrain(lightVal, 0, 1000), 0, 1000, 0, 30);
    display.drawRect(91, 48, 32, 7, SSD1306_WHITE);
    display.fillRect(92, 49, lightBar, 5, SSD1306_WHITE);

    display.display(); // Đẩy buffer lên màn hình thật
  }

  /**
   * @brief Vẽ overlay thông số nhỏ bên phải màn hình (dùng khi DANGER_FIRE).
   *
   * Dải nhỏ bên phải (x: 97-127):
   *  T / 45C   <- Nhiệt độ
   *  H / 50%   <- Độ ẩm
   *  L / 350   <- Ánh sáng
   *
   * @param disp Con trỏ đến đối tượng màn hình để vẽ lên
   */
  void drawMetricsOverlay(Adafruit_SSD1306* disp) {
    if (sensors == nullptr) return;

    disp->drawFastVLine(97, 0, 64, SSD1306_WHITE); // Đường kẻ dọc phân cách
    disp->setTextSize(1);
    disp->setTextColor(SSD1306_WHITE);

    // Nhiệt độ
    disp->setCursor(101, 4);  disp->print(F("T"));
    disp->setCursor(101, 12); disp->print((int)round(sensors->getTemperature())); disp->print(F("C"));

    // Độ ẩm
    disp->setCursor(101, 24); disp->print(F("H"));
    disp->setCursor(101, 32); disp->print((int)round(sensors->getHumidity())); disp->print(F("%"));

    // Ánh sáng
    disp->setCursor(101, 44); disp->print(F("L"));
    disp->setCursor(101, 52); disp->print((int)round(sensors->getLightLux()));
  }

  /**
   * @brief Cập nhật màn hình. Gọi liên tục trong loop().
   *
   * Thứ tự xử lý:
   *  1. Kiểm tra kết thúc wink
   *  2. Nếu đang hiện màn hình thông số -> vẽ thông số, thoát
   *  3. Nếu DANGER_FIRE -> vẽ mặt X_X thủ công + overlay thông số
   *  4. Ngược lại -> cập nhật RoboEyes (bao gồm mắt quay tròn nếu đang nhảy)
   */
  void update() {
    unsigned long now = millis();

    // --- Bước 1: Kết thúc wink nếu đã hết thời gian ---
    if (isWinking && now >= winkEndTime) {
      isWinking = false;
      setExpression(currentState); // Khôi phục biểu cảm bình thường
    }

    // --- Bước 2: Hiển thị màn hình thông số nếu được yêu cầu ---
    if (showParamScreen) {
      drawParamScreen();
      return; // Không vẽ mặt nữa
    }

    // --- Bước 3: Vẽ mặt X_X khi DANGER_FIRE ---
    if (currentState == DANGER_FIRE) {
      display.clearDisplay();

      // Mắt trái: Hình chữ X, tâm tại (28, 32)
      display.drawLine(16, 20, 40, 44, SSD1306_WHITE); // Đường chéo xuống phải
      display.drawLine(40, 20, 16, 44, SSD1306_WHITE); // Đường chéo xuống trái

      // Mắt phải: Hình chữ X, tâm tại (68, 32)
      display.drawLine(56, 20, 80, 44, SSD1306_WHITE);
      display.drawLine(80, 20, 56, 44, SSD1306_WHITE);

      // Miệng: Dấu gạch ngang _, tâm tại (48, 48)
      display.drawLine(43, 48, 53, 48, SSD1306_WHITE);

      // Vẽ thêm thông số nhỏ bên phải màn hình
      drawMetricsOverlay(&display);

      display.display();
      return;
    }

    // --- Bước 4: Cập nhật RoboEyes (mặt động hoạt hình) ---
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
