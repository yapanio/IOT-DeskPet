/**
 * @file controller.h
 * @brief BỘ NÃO TRUNG TÂM của robot - Kết nối tất cả các module lại với nhau.
 *
 * Đây là file quan trọng nhất. Class Controller đóng vai trò như một "nhạc trưởng"
 * điều phối toàn bộ hoạt động của robot theo mô hình MVC (Model-View-Controller):
 *
 *  MODEL (Dữ liệu):
 *    - sensors.h : Dữ liệu cảm biến (nhiệt độ, độ ẩm, ánh sáng)
 *    - robot_state.h : Trạng thái hiện tại của robot
 *
 *  VIEW (Hiển thị):
 *    - emote.h   : Màn hình OLED - mặt robot và thông số
 *    - actuators.h : Đèn LED NeoPixel
 *
 *  CONTROLLER (Điều khiển):
 *    - controller.h (file này!): Đọc dữ liệu và quyết định làm gì
 *    - robot_servo.h : Điều khiển đầu robot
 *    - blynk_service.h : Giao tiếp IoT Cloud
 *
 * CHỨC NĂNG CHÍNH:
 *  1. Đọc cảm biến -> Quyết định trạng thái robot
 *  2. Xử lý cử chỉ chạm (1 chạm, 2 chạm, 3 chạm, giữ lâu)
 *  3. Quản lý chế độ nhảy múa (Dance Mode)
 *  4. Phát âm thanh cảnh báo định kỳ
 *  5. Đồng bộ số liệu lên Blynk IoT Cloud
 *
 * PIN ĐỒ KẾT NỐI:
 *  GPIO 15 -> Cảm biến chạm điện dung (Touch Sensor)
 *  GPIO 19 -> DHT11 (Nhiệt độ & Độ ẩm)
 *  GPIO 14 -> Servo SG90 (Đầu robot)
 *  GPIO  4 -> NeoPixel LED (Vòng đèn)
 *  GPIO 18 -> Buzzer (Còi)
 *  SDA/SCL -> BH1750 + OLED (tự động qua Wire.begin())
 */

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "actuators.h"      // LED NeoPixel + Buzzer
#include "blynk_service.h"  // Kết nối Blynk IoT
#include "emote.h"          // Màn hình OLED + Khuôn mặt
#include "robot_servo.h"    // Điều khiển Servo
#include "robot_state.h"    // Định nghĩa các trạng thái
#include "sensors.h"        // Cảm biến môi trường
#include <Arduino.h>        // Thư viện chuẩn Arduino

// ===================================================
// ĐỊNH NGHĨA CÁC CHÂN GPIO
// ===================================================
#define TOUCH_PIN  15  // Cảm biến chạm điện dung
#define DHT_PIN    19  // Cảm biến nhiệt độ & độ ẩm DHT11
#define SERVO_PIN  14  // Động cơ servo đầu robot
#define LED_PIN     4  // Vòng đèn LED NeoPixel
#define BUZZER_PIN 18  // Còi Buzzer

/**
 * @class Controller
 * @brief Bộ não trung tâm của robot, điều phối mọi hoạt động.
 *
 * Cách dùng (trong Project.ino):
 *   Controller robotController;
 *   robotController.begin();  // Gọi trong setup()
 *   robotController.update(); // Gọi lặp lại trong loop()
 */
class Controller {
 private:
  // ===================================================
  // CÁC MODULE CON (Sub-systems)
  // ===================================================
  Sensors      sensors;   // Module cảm biến môi trường
  RobotServo   servo;     // Module điều khiển servo đầu
  Actuators    actuators; // Module LED + Buzzer
  Emote        emote;     // Module màn hình OLED + mặt robot
  BlynkService blynk;     // Module kết nối Blynk IoT

  // ===================================================
  // TRẠNG THÁI TỔNG THỂ CỦA ROBOT
  // ===================================================
  RobotState currentState = NORMAL_HAPPY; // Trạng thái hiện tại (bắt đầu là vui vẻ)
  bool testModeActive = false; // Có đang ở chế độ test (giả lập) không?
  bool alarmMuted     = false; // Cảnh báo âm thanh có đang bị tắt không?

  // ===================================================
  // BỘ ĐẾM THỜI GIAN CHO CẢNH BÁO
  // ===================================================
  // Dùng để nhớ thời điểm lần cuối để so sánh (non-blocking timing)
  unsigned long lastTimeLightChecked  = 0; // Thời điểm lux bắt đầu ở vùng cảnh báo tối
  unsigned long lastTimeBuzzerSounded = 0; // Thời điểm phát âm cảnh báo lần cuối

  // ===================================================
  // XỬ LÝ CẢM ỨNG CHẠM (Touch Gesture Detection)
  // ===================================================
  // Blynk có thể gửi lệnh "chạm ảo" qua V6 (Virtual Pin 6).
  // Robot xử lý cả chạm thật (GPIO) và chạm ảo (Blynk) giống nhau.
  bool blynkTouchState = false; // Trạng thái chạm từ Blynk
  bool lastTouchState  = false; // Trạng thái chạm của vòng lặp trước (để phát hiện cạnh)

  unsigned long touchStartTime  = 0;     // Thời điểm bắt đầu giữ ngón tay
  unsigned long lastTapTime     = 0;     // Thời điểm chạm nhanh gần nhất
  int           tapCount        = 0;     // Số lần chạm nhanh đếm được
  bool          longPressDetected = false; // Đã phát hiện giữ lâu chưa?

  // ===================================================
  // DANCE MODE (Chế độ nhảy múa)
  // ===================================================
  bool       isDancing    = false;       // Đang nhảy không?
  unsigned long danceEndTime = 0;        // Thời điểm kết thúc nhảy (ms)
  RobotState preDanceState = NORMAL_HAPPY; // Lưu trạng thái TRƯỚC KHI nhảy để khôi phục

  // ===================================================
  // TRẠNG THÁI MÀN HÌNH
  // ===================================================
  bool showParamScreen = false;              // Đang hiện màn hình thông số không?
  bool returnToParamScreenAfterDance = false; // Sau khi nhảy xong, có quay lại thông số không?

 public:
  /**
   * @brief Hàm khởi tạo của Controller.
   * Khai báo các module con với chân GPIO tương ứng.
   */
  Controller()
      : sensors(DHT_PIN),
        servo(SERVO_PIN),
        actuators(LED_PIN, BUZZER_PIN) {}

  /**
   * @brief Khởi động toàn bộ robot. Gọi MỘT LẦN duy nhất trong setup().
   *
   * Thứ tự khởi động:
   *  1. Serial debug (để xem log trên máy tính)
   *  2. Cấu hình chân GPIO
   *  3. Khởi động từng module con
   *  4. Kết nối WiFi + Blynk
   *  5. Áp dụng trạng thái ban đầu (NORMAL_HAPPY)
   */
  void begin() {
    // Khởi động cổng Serial để in thông tin debug ra máy tính
    Serial.begin(115200);
    while (!Serial && millis() < 3000); // Chờ Serial sẵn sàng (tối đa 3s)
    Serial.println(F("=========================================="));
    Serial.println(F("  IOT DeskPet - Khoi dong he thong..."));
    Serial.println(F("=========================================="));

    // Cấu hình chân cảm biến chạm là INPUT (đọc tín hiệu)
    pinMode(TOUCH_PIN, INPUT);

    // Khởi động từng module con
    sensors.begin();             // Cảm biến DHT11 + BH1750
    servo.begin();               // Servo SG90
    actuators.begin();           // LED NeoPixel + Buzzer
    emote.begin(&sensors);       // Màn hình OLED (cần truyền sensors để hiện thông số)
    blynk.begin();               // WiFi + Blynk (có thể mất đến 10 giây)

    // Áp dụng trạng thái ban đầu cho tất cả module
    applyStateToSubsystems();

    // Khởi tạo bộ đếm thời gian
    lastTimeLightChecked  = millis();
    lastTimeBuzzerSounded = millis();

    Serial.println(F("Robot Controller fully initialized."));
  }

  /**
   * @brief Vòng lặp chính của robot. Gọi LIÊN TỤC trong loop().
   *
   * Mỗi lần gọi update(), robot thực hiện 6 việc theo thứ tự:
   *  1. Cập nhật số liệu cảm biến
   *  2. Kiểm tra Dance Mode đã kết thúc chưa
   *  3. Đánh giá trạng thái môi trường và phát âm cảnh báo
   *  4. Xử lý cử chỉ chạm
   *  5. Cập nhật servo, LED, màn hình
   *  6. Gửi dữ liệu lên Blynk
   */
  void update() {
    // BƯỚC 1: Đọc cảm biến (non-blocking - chỉ đọc khi đến lúc)
    sensors.update();

    // BƯỚC 2: Kiểm tra xem Dance Mode đã hết giờ hoặc bài nhạc kết thúc chưa
    if (isDancing) {
      bool songEnded      = !actuators.isSongPlaying();
      bool danceTimeIsUp  = (millis() >= danceEndTime);
      if (danceTimeIsUp || songEnded) {
        Serial.println(F("[Dance Mode] Het thoi gian nhay. Tro ve binh thuong."));
        stopDanceMode();
      }
    }

    // BƯỚC 3: Đánh giá môi trường và phát âm cảnh báo (chỉ khi không nhảy)
    if (!isDancing) {
      evaluateEnvironmentAndUpdateState();
      updatePeriodicAlarmBeeps();
    }

    // BƯỚC 4: Xử lý cảm ứng chạm (1 chạm, 2 chạm, 3 chạm, giữ lâu)
    updateTouchGesture();

    // BƯỚC 5: Cập nhật phần cứng (servo/LED/OLED đều non-blocking)
    servo.update();
    actuators.update();
    emote.update();

    // BƯỚC 6: Gửi dữ liệu lên Blynk IoT Cloud
    blynk.update(
      sensors.getTemperature(),
      sensors.getHumidity(),
      sensors.getHeatIndex(),
      sensors.getLightLux(),
      stateToString(currentState)
    );
  }

  // ===================================================
  // CÁC HÀM PUBLIC (Được gọi từ Project.ino qua Blynk callbacks)
  // ===================================================

  /**
   * @brief Xử lý lệnh phát nhạc từ Blynk Virtual Pin V5.
   * Được gọi từ BLYNK_WRITE(V5) trong Project.ino.
   * @param songId 1=Mario, 2=Despacito, 3=Jingle Bells, 0=Dừng
   */
  void playSongBlynk(int songId) {
    if (songId >= 1 && songId <= 3) {
      triggerDanceMode(songId); // Bắt đầu nhảy với bài nhạc tương ứng
    } else {
      stopDanceMode(); // Dừng nhảy
    }
  }

  /**
   * @brief Cập nhật trạng thái chạm ảo từ Blynk Virtual Pin V6.
   * Được gọi từ BLYNK_WRITE(V6) trong Project.ino.
   * @param pressed true = đang nhấn, false = đã thả
   */
  void setBlynkTouch(bool pressed) {
    blynkTouchState = pressed;
    Serial.print(F("[Blynk] Cham ao: "));
    Serial.println(blynkTouchState ? F("NHAN") : F("THA"));
  }

 private:
  // ===================================================
  // QUẢN LÝ TRẠNG THÁI (State Management)
  // ===================================================

  /**
   * @brief Áp dụng trạng thái hiện tại cho tất cả các module con.
   * Gọi hàm này sau mỗi lần thay đổi currentState để đồng bộ
   * màu đèn, biểu cảm mặt, và chuyển động servo.
   */
  void applyStateToSubsystems() {
    servo.setState(currentState);       // Cập nhật chuyển động đầu servo
    actuators.setState(currentState);   // Cập nhật LED + tắt âm cũ
    emote.setExpression(currentState);  // Cập nhật biểu cảm mặt OLED
  }

  /**
   * @brief Chuyển đổi sang trạng thái mới một cách có kiểm soát.
   *
   * Chỉ thực sự chuyển đổi nếu trạng thái MỚI KHÁC trạng thái CŨ.
   * In log ra Serial để dễ debug.
   *
   * @param newState   Trạng thái mới muốn chuyển sang
   * @param resetAlarm true = xóa trạng thái mute (mặc định), false = giữ nguyên
   */
  void changeState(RobotState newState, bool resetAlarm = true) {
    if (currentState == newState) return; // Không làm gì nếu không đổi

    Serial.print(F("[State] "));
    Serial.print(stateToString(currentState));
    Serial.print(F(" -> "));
    Serial.println(stateToString(newState));

    currentState = newState;
    if (resetAlarm) alarmMuted = false; // Xóa mute khi đổi trạng thái

    applyStateToSubsystems();
  }

  // ===================================================
  // ĐÁNH GIÁ MÔI TRƯỜNG (Environment Evaluation)
  // ===================================================

  /**
   * @brief Đọc số liệu cảm biến và quyết định trạng thái phù hợp.
   *
   * ĐÂY LÀ LOGIC TRUNG TÂM CỦA ROBOT!
   *
   * Cây ưu tiên trạng thái (từ cao đến thấp):
   *  1. DANGER_FIRE  : Nhiệt độ > 42°C         -> Nguy hiểm cháy nổ
   *  2. DANGER_HUMID : Độ ẩm > 85%             -> Nguy hiểm ẩm mốc
   *  3. WARNING_HOT  : Temp > 35°C hoặc HI > 38°C -> Cảnh báo nóng
   *  4. WARNING_COLD : Temp < 18°C và Humid < 36%  -> Cảnh báo lạnh
   *  5. SLEEP_MODE   : Ánh sáng < 50 lux       -> Quá tối -> ngủ
   *  6. WARNING_DARK : Ánh sáng < 150 lux liên tục 10 phút -> Thiếu sáng
   *  7. NORMAL_HAPPY : Tất cả đều ổn           -> Vui vẻ!
   *
   * Khi trạng thái thay đổi, robot cũng tự động phát bài nhạc phù hợp.
   */
  void evaluateEnvironmentAndUpdateState() {
    float temp  = sensors.getTemperature(); // Nhiệt độ (°C)
    float humid = sensors.getHumidity();    // Độ ẩm (%)
    float feel  = sensors.getHeatIndex();   // Cảm giác nóng (°C)
    float lux   = sensors.getLightLux();    // Ánh sáng (lux)
    unsigned long now = millis();

    // --- Xác định trạng thái mục tiêu ---
    RobotState targetState = NORMAL_HAPPY; // Giả sử bình thường, rồi kiểm tra

    // Reset bộ đếm ánh sáng khi lux ra ngoài vùng 50-150 (vùng cảnh báo tối)
    if (lux >= 150.0 || lux < 50.0) {
      lastTimeLightChecked = now;
    }

    // Kiểm tra theo thứ tự ưu tiên từ cao xuống thấp
    if (temp > 42.0) {
      targetState = DANGER_FIRE;     // Ưu tiên cao nhất: cháy!
    } else if (humid > 85.0) {
      targetState = DANGER_HUMID;    // Ưu tiên 2: ẩm nguy hiểm
    } else if (temp > 35.0 || feel > 38.0) {
      targetState = WARNING_HOT;     // Ưu tiên 3: nóng
    } else if (temp < 18.0 && humid < 36.0) {
      targetState = WARNING_COLD;    // Ưu tiên 4: lạnh và khô
    } else if (lux < 50.0) {
      targetState = SLEEP_MODE;      // Ưu tiên 5: quá tối -> ngủ
    } else if (lux < 150.0) {
      // Ánh sáng yếu: chỉ cảnh báo nếu đã yếu liên tục hơn 10 phút
      if (now - lastTimeLightChecked > 600000) { // 600000ms = 10 phút
        targetState = WARNING_DARK;
      } else {
        // Đang trong vùng 50-150 lux nhưng chưa đủ 10 phút
        // Giữ nguyên WARNING_DARK nếu đang ở đó, hoặc về NORMAL_HAPPY
        targetState = (currentState == WARNING_DARK) ? WARNING_DARK : NORMAL_HAPPY;
      }
    } else {
      targetState = NORMAL_HAPPY; // Mọi thứ đều ổn!
    }

    // --- Chuyển trạng thái và cấu hình âm thanh kèm theo ---
    if (targetState != currentState) {
      changeState(targetState, true);

      // Khi vừa vào trạng thái mới, thiết lập âm thanh ban đầu
      if (currentState == WARNING_HOT) {
        // Phát Despacito ngay lập tức khi bắt đầu WARNING_HOT
        // (rồi cứ 5 phút beep 1 lần ở updatePeriodicAlarmBeeps)
        lastTimeBuzzerSounded = now - 300000; // Trick: đặt "5 phút trước" để beep ngay
        actuators.playSong(2); // Bài 2: Despacito

      } else if (currentState == WARNING_COLD) {
        // Phát Jingle Bells ngay lập tức khi bắt đầu WARNING_COLD
        lastTimeBuzzerSounded = now;
        actuators.playSong(3); // Bài 3: Jingle Bells

      } else if (currentState == WARNING_DARK) {
        // Chuẩn bị để beep 2 lần đầu tiên (delay 1 phút)
        lastTimeBuzzerSounded = now - 60000; // Trick: đặt "1 phút trước" để beep ngay
        actuators.stopSong();

      } else {
        // Các trạng thái khác: tắt nhạc, reset bộ đếm beep
        lastTimeBuzzerSounded = now;
        actuators.stopSong();
      }
    }
  }

  /**
   * @brief Phát tiếng beep cảnh báo định kỳ (không liên tục, không chặn).
   *
   * Chỉ hoạt động khi:
   *  - Không bị mute (alarmMuted = false)
   *  - Không có bài nhạc nào đang phát
   *  - Đang ở trạng thái WARNING_HOT hoặc WARNING_DARK
   *
   * Lịch beep:
   *  - WARNING_HOT  : 1 tiếng beep mỗi 5 phút (nhắc nhở không quá phiền)
   *  - WARNING_DARK : 2 tiếng beep mỗi 1 phút (nhắc bật đèn!)
   */
  void updatePeriodicAlarmBeeps() {
    // Không beep nếu: đang tắt âm HOẶC đang phát nhạc
    if (alarmMuted || actuators.isSongPlaying()) return;

    unsigned long now = millis();

    if (currentState == WARNING_HOT) {
      // Beep 1 lần mỗi 5 phút (300,000ms)
      if (now - lastTimeBuzzerSounded >= 300000) {
        lastTimeBuzzerSounded = now;
        actuators.triggerSingleBeep(1200, 150); // 1200Hz, 150ms
      }
    } else if (currentState == WARNING_DARK) {
      // Beep 2 lần mỗi 1 phút (60,000ms)
      if (now - lastTimeBuzzerSounded >= 60000) {
        lastTimeBuzzerSounded = now;
        actuators.triggerDoubleBeep(1500, 80, 80); // 1500Hz, 2 lần 80ms, khoảng nghỉ 80ms
      }
    }
  }

  // ===================================================
  // DANCE MODE (Chế độ nhảy múa)
  // ===================================================

  /**
   * @brief Kích hoạt chế độ nhảy múa với bài nhạc chỉ định.
   *
   * Khi Dance Mode bật:
   *  - Robot chuyển sang trạng thái DANCE_MODE
   *  - LED đổi sang hiệu ứng cầu vồng xoay
   *  - Mặt OLED hiện mắt xoay tròn
   *  - Servo lắc đầu nhanh
   *  - Còi phát nhạc
   *  - Hẹn giờ tối đa 10 giây (hoặc bài nhạc kết thúc)
   *
   * @param songId 1=Mario, 2=Despacito, 3=Jingle Bells
   */
  void triggerDanceMode(int songId = 1) {
    Serial.print(F("[Dance Mode] Bat dau nhay voi bai so: "));
    Serial.println(songId);

    isDancing     = true;
    danceEndTime  = millis() + 10000; // Tối đa 10 giây
    preDanceState = currentState;     // Lưu trạng thái hiện tại để phục hồi

    changeState(DANCE_MODE, false); // false = không reset mute
    actuators.playSong(songId);
  }

  /**
   * @brief Dừng chế độ nhảy múa và khôi phục trạng thái trước đó.
   *
   * Sau khi dừng:
   *  - Robot quay về trạng thái trước khi nhảy (preDanceState)
   *  - Nếu trước khi nhảy đang hiện màn hình thông số -> khôi phục lại
   */
  void stopDanceMode() {
    if (!isDancing) return; // Không làm gì nếu không đang nhảy

    Serial.println(F("[Dance Mode] Dung nhay."));
    isDancing = false;
    actuators.stopSong();

    changeState(preDanceState, false); // Khôi phục trạng thái trước khi nhảy

    // Khôi phục màn hình thông số nếu trước đó đang hiện
    if (returnToParamScreenAfterDance) {
      showParamScreen = true;
      emote.setShowParamScreen(true);
      returnToParamScreenAfterDance = false;
    }
  }

  // ===================================================
  // XỬ LÝ CỬ CHỈ CHẠM (Touch Gesture Handling)
  // ===================================================

  /**
   * @brief Xử lý khi người dùng CHẠM 1 LẦN nhanh.
   *
   * Trong chế độ TEST:
   *  - Mỗi lần chạm 1 lần sẽ chuyển sang trạng thái giả lập tiếp theo
   *  - Thứ tự: NORMAL -> FIRE -> HUMID -> HOT -> COLD -> SLEEP -> DARK -> NORMAL
   *  - Dùng để demo mà không cần tạo điều kiện thật
   *
   * Trong chế độ BÌNH THƯỜNG:
   *  - Nếu đang có cảnh báo kêu: TẮT TIẾNG cảnh báo
   *  - Nếu không có cảnh báo: Bật/tắt màn hình thông số
   */
  void handleSingleTap() {
    // --- Chế độ Test: Quay vòng trạng thái giả lập ---
    if (testModeActive) {
      Serial.println(F("[Test Mode] Cham 1 lan -> Chuyen trang thai gia lap."));

      // Xác định bộ giá trị cảm biến giả để gây ra trạng thái tiếp theo
      switch (currentState) {
        case NORMAL_HAPPY:
          sensors.setMock(true, 45.0, 50.0, 350.0); // Temp 45°C -> DANGER_FIRE
          Serial.println(F("[Test] Gia lap DANGER_FIRE (Nhiet do 45C)."));
          break;
        case DANGER_FIRE:
          sensors.setMock(true, 25.0, 90.0, 350.0); // Humid 90% -> DANGER_HUMID
          Serial.println(F("[Test] Gia lap DANGER_HUMID (Do am 90%)."));
          break;
        case DANGER_HUMID:
          sensors.setMock(true, 36.0, 50.0, 350.0); // Temp 36°C -> WARNING_HOT
          Serial.println(F("[Test] Gia lap WARNING_HOT (Nhiet do 36C)."));
          break;
        case WARNING_HOT:
          sensors.setMock(true, 15.0, 30.0, 350.0); // Temp 15°C, Humid 30% -> WARNING_COLD
          Serial.println(F("[Test] Gia lap WARNING_COLD (Nhiet 15C, Am 30%)."));
          break;
        case WARNING_COLD:
          sensors.setMock(true, 25.0, 55.0, 20.0); // Lux 20 -> SLEEP_MODE
          Serial.println(F("[Test] Gia lap SLEEP_MODE (Anh sang 20 lux)."));
          break;
        case SLEEP_MODE:
          sensors.setMock(true, 25.0, 55.0, 100.0); // Lux 100 + đặt timer = WARNING_DARK
          lastTimeLightChecked = millis() - 605000;  // Giả vờ đã tối 10 phút rồi
          Serial.println(F("[Test] Gia lap WARNING_DARK (Anh sang 100 lux, da 10 phut)."));
          break;
        case WARNING_DARK:
        default:
          sensors.setMock(false, 0, 0, 0); // Tắt giả lập -> về cảm biến thật
          Serial.println(F("[Test] Tat gia lap -> Doc cam bien that."));
          break;
      }

      // Reset mute để nghe lại âm thanh cảnh báo
      alarmMuted = false;
      actuators.setMuted(false);
      return;
    }

    // --- Chế độ Bình thường: Tắt tiếng hoặc bật thông số ---
    Serial.println(F("[Touch] Cham 1 lan trong che do binh thuong."));

    // Kiểm tra xem có đang có cảnh báo đang phát tiếng không
    bool alarmIsActive = !alarmMuted && (
        currentState == DANGER_FIRE  ||
        currentState == DANGER_HUMID ||
        actuators.isSongPlaying()
    );

    if (alarmIsActive) {
      // Tắt tiếng cảnh báo (Mute)
      Serial.println(F("[Touch] Tat tieng canh bao."));
      alarmMuted = true;
      actuators.setMuted(true);
    } else {
      // Bật/tắt màn hình thông số môi trường
      showParamScreen = !showParamScreen;
      emote.setShowParamScreen(showParamScreen);
      Serial.print(F("[Touch] Man hinh thong so: "));
      Serial.println(showParamScreen ? F("BAT") : F("TAT"));

      actuators.triggerSingleBeep(1500, 100); // Beep xác nhận
    }
  }

  /**
   * @brief Xử lý khi người dùng CHẠM 2 LẦN NHANH.
   *
   * Robot thực hiện:
   *  - 2 tiếng beep cao
   *  - Nháy mắt (Wink) 1 giây
   *  - Lắc đầu nhẹ nhàng 1 giây
   *
   * Không hoạt động khi đang hiện màn hình thông số.
   */
  void handleDoubleTap() {
    if (showParamScreen) {
      Serial.println(F("[Touch] Cham 2 lan khi dang hien thi thong so - bo qua."));
      return;
    }
    Serial.println(F("[Touch] Cham 2 lan -> Nham mat + Lac dau."));
    actuators.triggerDoubleBeep(2500, 60, 60); // 2 beep tần số cao
    emote.triggerWink();                        // Nháy mắt
    servo.triggerGentleShake();                 // Lắc đầu
  }

  /**
   * @brief Xử lý khi người dùng CHẠM 3 LẦN NHANH.
   *
   * Kích hoạt Dance Mode với bài Super Mario (bài 1) trong 5 giây.
   * Nếu đang hiện màn hình thông số, sau khi nhảy xong sẽ quay lại màn hình thông số.
   */
  void handleTripleTap() {
    Serial.println(F("[Touch] Cham 3 lan -> Kich hoat Dance Mode!"));

    // Ghi nhớ và ẩn màn hình thông số trước khi nhảy
    if (showParamScreen) {
      returnToParamScreenAfterDance = true;
      showParamScreen               = false;
      emote.setShowParamScreen(false);
    } else {
      returnToParamScreenAfterDance = false;
    }

    triggerDanceMode(1); // Nhảy với bài Mario (ID = 1)
    danceEndTime = millis() + 5000; // Giới hạn 5 giây khi chạm 3 lần
  }

  /**
   * @brief Phát hiện và phân tích cử chỉ chạm từ cảm biến vật lý và Blynk.
   *
   * Gọi liên tục trong update(). Hàm này hoạt động như một STATE MACHINE:
   *
   * STATE MACHINE CỬ CHỈ CHẠM:
   *  ┌──────────────────────────────────────────────────────────────────┐
   *  │  Phát hiện CẠN TĂNG (Bắt đầu chạm):                            │
   *  │    -> Ghi nhớ thời điểm bắt đầu, reset longPressDetected        │
   *  │                                                                  │
   *  │  Trong khi đang giữ:                                            │
   *  │    -> Nếu giữ >= 2 giây: Bật/tắt Test Mode (Long Press)         │
   *  │                                                                  │
   *  │  Phát hiện CẠN GIẢM (Thả tay):                                 │
   *  │    -> Nếu chạm 50-600ms và không phải long press: tapCount++    │
   *  │                                                                  │
   *  │  Sau khi thả tay 400ms (không chạm thêm):                       │
   *  │    -> tapCount == 1: handleSingleTap()                           │
   *  │    -> tapCount == 2: handleDoubleTap()                           │
   *  │    -> tapCount >= 3: handleTripleTap()                           │
   *  └──────────────────────────────────────────────────────────────────┘
   *
   * Không xử lý chạm khi đang Dance Mode (để tránh gián đoạn).
   */
  void updateTouchGesture() {
    if (isDancing) return; // Bỏ qua chạm khi đang nhảy

    unsigned long now = millis();

    // Đọc trạng thái chạm: cảm biến vật lý OR chạm ảo từ Blynk
    bool isTouched = (digitalRead(TOUCH_PIN) == HIGH) || blynkTouchState;

    // --- Phát hiện cạnh TĂNG (vừa chạm vào) ---
    if (isTouched && !lastTouchState) {
      touchStartTime    = now;
      longPressDetected = false; // Reset trạng thái long press
    }

    // --- Phát hiện cạnh GIẢM (vừa thả tay) ---
    if (!isTouched && lastTouchState) {
      unsigned long pressDuration = now - touchStartTime;

      // Chỉ tính là TAP nếu:
      //  - Thời gian chạm từ 50ms đến 600ms (không quá nhanh, không quá lâu)
      //  - Không phải là kết quả của long press
      if (!longPressDetected && pressDuration > 50 && pressDuration < 600) {
        tapCount++;
        lastTapTime = now; // Ghi nhớ thời điểm tap gần nhất
      }
    }

    // --- Phát hiện LONG PRESS (Giữ lâu >= 2 giây) ---
    if (isTouched && !longPressDetected) {
      unsigned long pressDuration = now - touchStartTime;
      if (pressDuration >= 2000) { // Giữ đủ 2 giây
        longPressDetected = true;
        tapCount          = 0; // Hủy bỏ bất kỳ tap nào đang chờ

        // Bật/tắt chế độ Test
        testModeActive = !testModeActive;
        Serial.print(F("[Touch] Giu lau -> Che do Test: "));
        Serial.println(testModeActive ? F("BAT") : F("TAT"));

        if (testModeActive) {
          // Bật Test Mode: 2 beep báo hiệu
          alarmMuted = false;
          actuators.setMuted(false);
          actuators.triggerDoubleBeep(2000, 100, 100);
        } else {
          // Tắt Test Mode: Tắt giả lập, quay về cảm biến thật, 1 beep dài
          sensors.setMock(false, 0, 0, 0);
          alarmMuted = false;
          actuators.setMuted(false);
          actuators.triggerSingleBeep(1000, 200);
        }
      }
    }

    // --- Phân tích số lần TAP sau khoảng nghỉ 400ms ---
    // Chờ 400ms sau lần tap cuối để chắc chắn người dùng không tap thêm
    if (tapCount > 0 && (now - lastTapTime > 400)) {
      if      (tapCount == 1) handleSingleTap();
      else if (tapCount == 2) handleDoubleTap();
      else if (tapCount >= 3) handleTripleTap();
      tapCount = 0; // Reset bộ đếm tap
    }

    // Lưu lại trạng thái chạm của lần này để dùng cho lần sau
    lastTouchState = isTouched;
  }

  // ===================================================
  // TIỆN ÍCH (Utility Functions)
  // ===================================================

  /**
   * @brief Chuyển đổi giá trị RobotState thành chuỗi text để in log.
   * Ví dụ: DANGER_FIRE -> "DANGER_FIRE"
   * @param state Trạng thái cần chuyển đổi
   * @return Chuỗi text tương ứng
   */
  const char* stateToString(RobotState state) {
    switch (state) {
      case DANGER_FIRE:   return "DANGER_FIRE";
      case DANGER_HUMID:  return "DANGER_HUMID";
      case WARNING_HOT:   return "WARNING_HOT";
      case WARNING_COLD:  return "WARNING_COLD";
      case WARNING_DARK:  return "WARNING_DARK";
      case SLEEP_MODE:    return "SLEEP_MODE";
      case NORMAL_HAPPY:  return "NORMAL_HAPPY";
      case DANCE_MODE:    return "DANCE_MODE";
      default:            return "UNKNOWN";
    }
  }
};

#endif  // CONTROLLER_H
