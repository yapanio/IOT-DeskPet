/**
 * @file controller.h
 * @brief BỘ NÃO NHẠC TRƯỞNG - Kết nối và điều khiển toàn bộ các bộ phận của Robot!
 *
 * Con hãy tưởng tượng chú Robot giống như một con búp bê thông minh có:
 *  - Làn da (cảm biến DHT11) biết cảm nhận nóng lạnh.
 *  - Đôi mắt (màn hình OLED) biết cười biết khóc.
 *  - Chiếc cổ (động cơ Servo) biết quay sang trái sang phải.
 *  - Trái tim (vòng đèn LED) biết nháy sáng nhịp thở.
 *  - Chiếc miệng (còi Buzzer) biết hú còi cứu hỏa và hát bài nhạc.
 *  - Bộ thu phát sóng ma thuật (WiFi + Blynk) để nói chuyện với điện thoại.
 *
 * File này chính là BỘ NÃO điều phối tất cả các bộ phận đó hoạt động ăn ý với nhau!
 *
 * PHÂN BỔ CỔNG CẮM (Pinout):
 *  - Chân GPIO 15: Cảm biến chạm (Touch Sensor)
 *  - Chân GPIO 19: Cảm biến nhiệt độ & độ ẩm DHT11
 *  - Chân GPIO 14: Động cơ quay cổ Servo SG90
 *  - Chân GPIO 4: Vòng đèn LED NeoPixel
 *  - Chân GPIO 18: Còi Buzzer phát âm thanh
 *  - Các chân SDA/SCL: Cắm mắt ánh sáng BH1750 và màn hình OLED SSD1306
 */

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "actuators.h"      // Thiết bị LED NeoPixel + còi Buzzer
#include "blynk_service.h"  // Hộp thư kết nối Blynk IoT
#include "emote.h"          // Biểu cảm mắt màn hình OLED
#include "robot_servo.h"    // Khớp cổ quay Servo
#include "robot_state.h"    // Bảng tâm trạng của robot
#include "sensors.h"        // Giác quan cảm biến DHT11 + BH1750
#include <Arduino.h>        // Thư viện cơ bản để lập trình mạch Arduino

// ===================================================
// KHAI BÁO CỔNG CẮM TRÊN BOARD ESP32
// ===================================================
#define TOUCH_PIN  15  // Cổng cắm cảm biến chạm
#define DHT_PIN    19  // Cổng cắm cảm biến nhiệt độ DHT11
#define SERVO_PIN  14  // Cổng cắm động cơ cổ Servo
#define LED_PIN     4  // Cổng cắm dây điều khiển đèn LED
#define BUZZER_PIN 18  // Cổng cắm dây còi Buzzer

/**
 * @class Controller
 * @brief Bộ não điều khiển trung tâm của robot.
 */
class Controller {
 private:
  // ===================================================
  // CÁC BỘ PHẬN TRÊN CƠ THỂ ROBOT
  // ===================================================
  Sensors      sensors;   // Giác quan đo đạc
  RobotServo   servo;     // Khớp cổ quay
  Actuators    actuators; // Đèn và còi phát nhạc
  Emote        emote;     // Khuôn mặt và màn hình OLED
  BlynkService blynk;     // Kết nối WiFi với điện thoại

  // ===================================================
  // CẢM XÚC VÀ CÁC CHẾ ĐỘ CỦA ROBOT
  // ===================================================
  RobotState currentState = NORMAL_HAPPY; // Lúc đầu robot sẽ vui vẻ hạnh phúc
  bool testModeActive = false; // Có đang giả vờ đóng kịch (Test Mode) không?
  bool alarmMuted     = false; // Có đang tắt tiếng còi báo động không?

  // ===================================================
  // HẸN GIỜ ĐỂ NHẮC NHỞ (Non-blocking Timer)
  // ===================================================
  unsigned long lastTimeLightChecked  = 0; // Thời gian bắt đầu vào phòng tối (ms)
  unsigned long lastTimeBuzzerSounded = 0; // Lần cuối kêu bíp bíp nhắc nhở là lúc nào? (ms)

  // ===================================================
  // NHẬN BIẾT BÉ CHẠM VÀO ROBOT (Touch Gestures)
  // ===================================================
  bool blynkTouchState = false; // Bé chạm ảo từ nút bấm trên điện thoại
  bool lastTouchState  = false; // Nhớ xem lần trước bé có chạm vào không để tính toán

  unsigned long touchStartTime  = 0;     // Bé bắt đầu đặt tay lên cảm biến từ lúc nào? (ms)
  unsigned long lastTapTime     = 0;     // Lần thả tay gần nhất là khi nào? (ms)
  int           tapCount        = 0;     // Đếm xem bé chạm nhanh mấy lần liên tục rồi?
  bool          longPressDetected = false; // Đã phát hiện bé ôm giữ lâu chưa?

  // ===================================================
  // DANCE MODE (Chế độ nhảy múa)
  // ===================================================
  bool       isDancing    = false;       // Robot đang nhảy múa không?
  unsigned long danceEndTime = 0;        // Nhảy đến mấy giờ thì kết thúc? (ms)
  RobotState preDanceState = NORMAL_HAPPY; // Nhớ cảm xúc cũ để nhảy xong thì quay lại cảm xúc đó

  // ===================================================
  // TRẠNG THÁI HIỂN THỊ MÀN HÌNH
  // ===================================================
  bool showParamScreen = false;              // Có đang hiện màn hình thông số đo đạc không?
  bool returnToParamScreenAfterDance = false; // Nhảy múa xong có cần hiện lại màn hình thông số không?

 public:
  /**
   * @brief Hàm lắp ráp các bộ phận robot vào đúng chân cắm tương ứng.
   */
  Controller()
      : sensors(DHT_PIN),
        servo(SERVO_PIN),
        actuators(LED_PIN, BUZZER_PIN) {}

  /**
   * @brief Đánh thức toàn bộ robot dậy. Gọi một lần duy nhất trong setup().
   */
  void begin() {
    Serial.begin(115200); // Mở đường truyền kết nối với máy tính để gửi tin nhắn debug
    while (!Serial && millis() < 3000); // Chờ cổng máy tính mở (chờ tối đa 3 giây)
    Serial.println(F("=========================================="));
    Serial.println(F("  Robot DeskPet bắt đầu thức dậy và khởi động..."));
    Serial.println(F("=========================================="));

    pinMode(TOUCH_PIN, INPUT); // Đặt chân cảm biến chạm là cổng nhận tín hiệu

    // Đánh thức tất cả các module con dậy hoạt động
    sensors.begin();             // Đo cảm biến
    servo.begin();               // Cổ servo
    actuators.begin();           // Đèn và loa còi
    emote.begin(&sensors);       // Màn hình đôi mắt
    blynk.begin();               // Kết nối WiFi (thử bắt sóng trong 10 giây)

    applyStateToSubsystems(); // Đồng bộ cảm xúc ban đầu là NORMAL_HAPPY

    // Bắt đầu đếm giờ
    lastTimeLightChecked  = millis();
    lastTimeBuzzerSounded = millis();

    Serial.println(F("Robot đã khởi động xong toàn bộ rồi!"));
  }

  /**
   * @brief Bộ não chạy liên tục. Gọi lặp lại mãi mãi trong loop().
   */
  void update() {
    // 1. Cập nhật thông số thời tiết đo đạc được
    sensors.update();

    // 2. Kiểm tra xem robot nhảy múa (Dance) đã mệt và hết giờ chưa
    if (isDancing) {
      bool songEnded      = !actuators.isSongPlaying(); // Nhạc đã tắt chưa?
      bool danceTimeIsUp  = (millis() >= danceEndTime); // Đã nhảy đủ 10 giây chưa?
      if (danceTimeIsUp || songEnded) {
        Serial.println(F("[Nhảy múa] Hết giờ rồi! Robot dừng nhảy để nghỉ ngơi."));
        stopDanceMode();
      }
    }

    // 3. Nếu robot không nhảy múa: Cảm nhận môi trường và thỉnh thoảng kêu bíp bíp nhắc nhở
    if (!isDancing) {
      evaluateEnvironmentAndUpdateState(); // Đọc cảm biến đổi cảm xúc
      updatePeriodicAlarmBeeps();          // Tiếng bíp bíp định kỳ nhắc nhở
    }

    // 4. Nhận biết bé chạm tay vào người để phản hồi winking, nhảy múa
    updateTouchGesture();

    // 5. Cập nhật hoạt động cho cổ quay, đèn LED và đôi mắt OLED
    servo.update();
    actuators.update();
    emote.update();

    // 6. Gửi báo cáo tình hình thời tiết lên điện thoại của con qua Blynk
    blynk.update(
      sensors.getTemperature(),
      sensors.getHumidity(),
      sensors.getHeatIndex(),
      sensors.getLightLux(),
      stateToString(currentState)
    );
  }

  // ===================================================
  // CÁC LỆNH GỌI TỪ ĐIỆN THOẠI (Blynk callbacks)
  // ===================================================

  /**
   * @brief Nhận bài hát từ điện thoại và bắt đầu nhảy.
   * @param songId 1=Mario, 2=Despacito, 3=Jingle Bells, 0=Dừng nhảy
   */
  void playSongBlynk(int songId) {
    if (songId >= 1 && songId <= 3) {
      triggerDanceMode(songId); // Nhảy theo bài hát đó
    } else {
      stopDanceMode(); // Tắt nhạc và đứng yên
    }
  }

  /**
   * @brief Nhận lệnh chạm ảo từ điện thoại.
   */
  void setBlynkTouch(bool pressed) {
    blynkTouchState = pressed;
    Serial.print(F("[Blynk] Bé chạm ảo: "));
    Serial.println(blynkTouchState ? F("CHẠM VÀO") : F("THẢ TAY"));
  }

 private:
  // ===================================================
  // ĐỒNG BỘ CẢM XÚC ROBOT
  // ===================================================

  /**
   * @brief Báo cho các bộ phận cổ quay, đèn led, đôi mắt biết cảm xúc hiện tại là gì để đồng bộ.
   */
  void applyStateToSubsystems() {
    servo.setState(currentState);       // Cổ quay theo hướng tương ứng
    actuators.setState(currentState);   // Đèn đổi màu tương ứng
    emote.setExpression(currentState);  // Mắt đổi biểu cảm tương ứng
  }

  /**
   * @brief Chuyển sang cảm xúc mới nếu thời tiết thay đổi.
   */
  void changeState(RobotState newState, bool resetAlarm = true) {
    if (currentState == newState) return; // Nếu cảm xúc vẫn thế thì thôi giữ nguyên

    Serial.print(F("[Cảm xúc đổi] Từ "));
    Serial.print(stateToString(currentState));
    Serial.print(F(" sang -> "));
    Serial.println(stateToString(newState));

    currentState = newState;
    if (resetAlarm) alarmMuted = false; // Đổi cảm xúc mới thì bật lại loa báo động

    applyStateToSubsystems(); // Báo cho các bộ phận thay đổi theo cảm xúc mới
  }

  // ===================================================
  // CẢM NHẬN THỜI TIẾT ĐỂ THAY ĐỔI CẢM XÚC (evaluate)
  // ===================================================

  /**
   * @brief Đọc cảm biến và tự động chọn tâm trạng phù hợp cho robot.
   */
  void evaluateEnvironmentAndUpdateState() {
    float temp  = sensors.getTemperature(); // Lấy nhiệt độ phòng
    float humid = sensors.getHumidity();    // Lấy độ ẩm phòng
    float feel  = sensors.getHeatIndex();   // Cảm giác nóng thực tế
    float lux   = sensors.getLightLux();    // Lấy ánh sáng phòng
    unsigned long now = millis();

    RobotState targetState = NORMAL_HAPPY; // Ban đầu giả vờ là rất vui vẻ hạnh phúc

    // Nếu phòng sáng hoặc tối om hẳn thì reset đồng hồ ánh sáng yếu
    if (lux >= 150.0 || lux < 50.0) {
      lastTimeLightChecked = now;
    }

    // --- KIỂM TRA ĐIỀU KIỆN THEO THỨ TỰ ƯU TIÊN ---
    if (temp > 42.0) {
      targetState = DANGER_FIRE;     // 1. Quá nóng > 42°C: Cháy rồi hoảng hốt kêu bíp bíp siren!
    } else if (humid > 85.0) {
      targetState = DANGER_HUMID;    // 2. Ẩm ướt > 85%: Ướt sũng hỏng mạch cứu tôi với!
    } else if (temp > 35.0 || feel > 38.0) {
      targetState = WARNING_HOT;     // 3. Nóng nực > 35°C: mệt mỏi
    } else if (temp < 18.0 && humid < 36.0) {
      targetState = WARNING_COLD;    // 4. Lạnh < 18°C: run cầm cập
    } else if (lux < 50.0) {
      targetState = SLEEP_MODE;      // 5. Tối om < 50 lux: Buồn ngủ quá nhắm mắt ngủ thôi
    } else if (lux < 150.0) {
      // 6. Ánh sáng yếu: Nếu yếu liên tục hơn 10 phút (600000ms) thì robot mới buồn bã WARNING_DARK
      if (now - lastTimeLightChecked > 600000) {
        targetState = WARNING_DARK;
      } else {
        // Chưa đủ 10 phút thì giữ nguyên tâm trạng cũ
        targetState = (currentState == WARNING_DARK) ? WARNING_DARK : NORMAL_HAPPY;
      }
    } else {
      targetState = NORMAL_HAPPY; // Môi trường hoàn hảo lý tưởng!
    }

    // --- Thực hiện chuyển đổi cảm xúc và bật bài hát dỗ dành tương ứng ---
    if (targetState != currentState) {
      changeState(targetState, true);

      // Thiết lập bài hát ngay lúc vừa bước vào cảm xúc mới
      if (currentState == WARNING_HOT) {
        lastTimeBuzzerSounded = now - 300000; // Trick: Đánh lừa bộ đếm thời gian để bíp nhắc nhở ngay lập tức
        actuators.playSong(2); // Tự động hát bài Despacito sôi động giải nhiệt!
      } else if (currentState == WARNING_COLD) {
        lastTimeBuzzerSounded = now;
        actuators.playSong(3); // Tự động hát bài Jingle Bells sưởi ấm mùa đông!
      } else if (currentState == WARNING_DARK) {
        lastTimeBuzzerSounded = now - 60000; // Trick: Hẹn giờ để kêu bíp bíp đôi ngay
        actuators.stopSong();
      } else {
        // Cảm xúc bình thường: tắt hát và im lặng
        lastTimeBuzzerSounded = now;
        actuators.stopSong();
      }
    }
  }

  /**
   * @brief Kêu bíp bíp định kỳ để nhắc nhở con chăm sóc robot.
   *
   * Chỉ kêu bíp bíp khi:
   *  - Loa không bị tắt (alarmMuted = false)
   *  - Robot không đang hát nhạc
   *  - Trời quá NÓNG hoặc quá TỐI
   */
  void updatePeriodicAlarmBeeps() {
    if (alarmMuted || actuators.isSongPlaying()) return;

    unsigned long now = millis();

    if (currentState == WARNING_HOT) {
      // Trời nóng: Cứ 5 phút (300000ms) lại kêu "Bíp" 1 tiếng ngắn nhắc con bật quạt
      if (now - lastTimeBuzzerSounded >= 300000) {
        lastTimeBuzzerSounded = now;
        actuators.triggerSingleBeep(1200, 150); // Bíp ở tần số 1200Hz trong 150ms
      }
    } else if (currentState == WARNING_DARK) {
      // Trời tối lâu: Cứ 1 phút (60000ms) lại kêu "Bíp Bíp" đôi nhắc con bật đèn kẻo hại mắt
      if (now - lastTimeBuzzerSounded >= 60000) {
        lastTimeBuzzerSounded = now;
        actuators.triggerDoubleBeep(1500, 80, 80); // Bíp đôi tần số 1500Hz
      }
    }
  }

  // ===================================================
  // KÍCH HOẠT NHẢY MÚA (Dance Mode)
  // ===================================================

  /**
   * @brief Bắt đầu cho robot nhảy múa vui vẻ.
   */
  void triggerDanceMode(int songId = 1) {
    Serial.print(F("[Nhảy múa] Bắt đầu nhảy múa theo nhạc bài số: "));
    Serial.println(songId);

    isDancing     = true;
    danceEndTime  = millis() + 10000; // Nhảy múa tối đa trong 10 giây
    preDanceState = currentState;     // Lưu lại cảm xúc trước đó để nhảy xong còn quay lại

    changeState(DANCE_MODE, false); // Đổi mắt và đèn sang chế độ nhảy múa (không reset loa)
    actuators.playSong(songId);     // Bắt đầu mở bài hát vui nhộn
  }

  /**
   * @brief Kết thúc nhảy múa, khôi phục lại cảm xúc cũ.
   */
  void stopDanceMode() {
    if (!isDancing) return;

    Serial.println(F("[Nhảy múa] Hết bài rồi, dừng nhảy múa thôi."));
    isDancing = false;
    actuators.stopSong(); // Tắt nhạc nếu chưa hết

    changeState(preDanceState, false); // Khôi phục lại cảm xúc trước khi nhảy

    // Nếu trước đó con đang mở màn hình thông số đo đạc thì hiện lại
    if (returnToParamScreenAfterDance) {
      showParamScreen = true;
      emote.setShowParamScreen(true);
      returnToParamScreenAfterDance = false;
    }
  }

  // ===================================================
  // ĐẾM SỐ LẦN BÉ CHẠM VÀO ROBOT (Touch Gesture)
  // ===================================================

  /**
   * @brief Bé chạm 1 lần nhanh:
   *  - Nếu đang có chuông kêu: Tắt âm báo động (Mute).
   *  - Nếu không kêu: Bật/Tắt màn hình xem số liệu thời tiết phòng.
   *  - Nếu đang ở chế độ Test (giả lập): Chuyển qua các thời tiết đóng kịch khác nhau.
   */
  void handleSingleTap() {
    // --- Chế độ Test: Chuyển kịch bản giả vờ ---
    if (testModeActive) {
      Serial.println(F("[Test giả lập] Chạm 1 lần -> Đổi kịch bản giả lập thời tiết."));

      switch (currentState) {
        case NORMAL_HAPPY:
          sensors.setMock(true, 45.0, 50.0, 350.0); // Giả vờ nhiệt độ 45°C -> Hú báo CHÁY!
          break;
        case DANGER_FIRE:
          sensors.setMock(true, 25.0, 90.0, 350.0); // Giả vờ độ ẩm 90% -> Báo ẨM ƯỚT!
          break;
        case DANGER_HUMID:
          sensors.setMock(true, 36.0, 50.0, 350.0); // Giả vờ nhiệt độ 36°C -> Cảnh báo NÓNG!
          break;
        case WARNING_HOT:
          sensors.setMock(true, 15.0, 30.0, 350.0); // Giả vờ lạnh 15°C -> Cảnh báo LẠNH!
          break;
        case WARNING_COLD:
          sensors.setMock(true, 25.0, 55.0, 20.0);  // Giả vờ tối om 20 lux -> Đi NGỦ!
          break;
        case SLEEP_MODE:
          sensors.setMock(true, 25.0, 55.0, 100.0); // Giả vờ ánh sáng yếu 100 lux và tối lâu 10 phút -> WARNING_DARK!
          lastTimeLightChecked = millis() - 605000;
          break;
        case WARNING_DARK:
        default:
          sensors.setMock(false, 0, 0, 0); // Thôi không giả vờ nữa -> Đọc cảm biến thật ngoài đời!
          break;
      }

      // Đổi kịch bản xong thì tự bật lại loa
      alarmMuted = false;
      actuators.setMuted(false);
      return;
    }

    // --- Chế độ thường: Tắt tiếng còi hoặc bật màn hình thông số ---
    Serial.println(F("[Cảm ứng] Chạm 1 lần nhanh!"));

    // Xem còi có đang rú hay nhạc có đang hát báo động không?
    bool alarmIsActive = !alarmMuted && (
        currentState == DANGER_FIRE  ||
        currentState == DANGER_HUMID ||
        actuators.isSongPlaying()
    );

    if (alarmIsActive) {
      // Tắt tiếng kêu (Im lặng)
      Serial.println(F("[Cảm ứng] Im lặng: Tắt tiếng còi cảnh báo."));
      alarmMuted = true;
      actuators.setMuted(true);
    } else {
      // Bật hoặc tắt màn hình xem thông số
      showParamScreen = !showParamScreen;
      emote.setShowParamScreen(showParamScreen);
      Serial.print(F("[Cảm ứng] Xem màn hình thông số: "));
      Serial.println(showParamScreen ? F("BẬT") : F("TẮT"));

      actuators.triggerSingleBeep(1500, 100); // Kêu "bíp" nhẹ 1 cái để báo hiệu đã bấm
    }
  }

  /**
   * @brief Bé chạm 2 lần liên tục: Nháy mắt nháy mắt trêu bé kết hợp lắc nhẹ đầu xinh!
   */
  void handleDoubleTap() {
    if (showParamScreen) {
      Serial.println(F("[Cảm ứng] Đang hiện màn hình thông số, bỏ qua chạm 2 lần."));
      return;
    }
    Serial.println(F("[Cảm ứng] Chạm 2 lần nhanh -> Nháy mắt + Lắc đầu chào bé!"));
    actuators.triggerDoubleBeep(2500, 60, 60); // Kêu bíp bíp nhanh
    emote.triggerWink();                        // Nháy một mắt winking
    servo.triggerGentleShake();                 // Lắc nhẹ cổ
  }

  /**
   * @brief Bé chạm 3 lần liên tục: Kích hoạt nhảy múa Dance Mode vui nhộn trong 5 giây!
   */
  void handleTripleTap() {
    Serial.println(F("[Cảm ứng] Chạm 3 lần -> Bắt đầu nhảy múa nào!"));

    // Ẩn tạm màn hình thông số đi để xem mặt robot nhảy múa
    if (showParamScreen) {
      returnToParamScreenAfterDance = true;
      showParamScreen               = false;
      emote.setShowParamScreen(false);
    } else {
      returnToParamScreenAfterDance = false;
    }

    triggerDanceMode(1); // Hát bài Mario để nhảy múa
    danceEndTime = millis() + 5000; // Nhảy nhanh 5 giây rồi thôi
  }

  /**
   * @brief Nhận biết cử chỉ gõ đầu chạm nhẹ hoặc ôm chặt giữ lâu.
   */
  void updateTouchGesture() {
    if (isDancing) return; // Nếu đang nhảy múa thì không nhận chạm để tránh bị làm phiền

    unsigned long now = millis();

    // Đọc tín hiệu chạm (từ dây chạm vật lý hoặc nút ảo điện thoại)
    bool isTouched = (digitalRead(TOUCH_PIN) == HIGH) || blynkTouchState;

    // --- Bé bắt đầu đặt ngón tay lên chạm (Cạnh tăng) ---
    if (isTouched && !lastTouchState) {
      touchStartTime    = now;
      longPressDetected = false;
    }

    // --- Bé thả tay ra không chạm nữa (Cạnh giảm) ---
    if (!isTouched && lastTouchState) {
      unsigned long pressDuration = now - touchStartTime;

      // Nếu bé chạm nhanh (từ 50 mili-giây đến 600 mili-giây) và không phải là đang giữ lâu
      if (!longPressDetected && pressDuration > 50 && pressDuration < 600) {
        tapCount++;
        lastTapTime = now; // Lưu thời gian lần chạm cuối
      }
    }

    // --- Bé ôm chặt giữ lâu liên tục 2 giây (Long Press) ---
    if (isTouched && !longPressDetected) {
      unsigned long pressDuration = now - touchStartTime;
      if (pressDuration >= 2000) { // Giữ đủ 2 giây
        longPressDetected = true;
        tapCount          = 0; // Hủy toàn bộ đếm chạm nhanh đang chờ

        // Bật hoặc tắt chế độ Test đóng kịch giả vờ
        testModeActive = !testModeActive;
        Serial.print(F("[Cảm ứng] Giữ lâu -> Chế độ Test: "));
        Serial.println(testModeActive ? F("MỞ") : F("TẮT"));

        if (testModeActive) {
          // Bật Test Mode: Kêu bíp bíp đôi báo hiệu sẵn sàng
          alarmMuted = false;
          actuators.setMuted(false);
          actuators.triggerDoubleBeep(2000, 100, 100);
        } else {
          // Tắt Test Mode: Quay về cảm biến thật, kêu 1 bíp dài chào tạm biệt
          sensors.setMock(false, 0, 0, 0);
          alarmMuted = false;
          actuators.setMuted(false);
          actuators.triggerSingleBeep(1000, 200);
        }
      }
    }

    // --- Phân tích số lần gõ sau khi thả tay được 400ms (chắc chắn bé đã gõ xong) ---
    if (tapCount > 0 && (now - lastTapTime > 400)) {
      if      (tapCount == 1) handleSingleTap();
      else if (tapCount == 2) handleDoubleTap();
      else if (tapCount >= 3) handleTripleTap();
      tapCount = 0; // Reset đếm số lần gõ về 0
    }

    lastTouchState = isTouched; // Nhớ trạng thái chạm lần này
  }

  // ===================================================
  // CÁC HÀM TIỆN ÍCH
  // ===================================================
  /**
   * @brief Đổi tên trạng thái cảm xúc thành chữ viết thường để in log ra màn hình máy tính.
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
