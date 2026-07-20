/**
 * @file robot_servo.h
 * @brief Chiếc cổ của Robot - Giúp Robot biết quay đầu ngó nghiêng!
 *
 * Robot có một động cơ Servo nhỏ giống như cái khớp cổ của chúng ta:
 *  - Góc 90°: Robot nhìn thẳng phía trước.
 *  - Góc nhỏ hơn 90° (như 60°): Robot quay đầu sang bên trái.
 *  - Góc lớn hơn 90° (như 120°): Robot quay đầu sang bên phải.
 *
 * ĐẶC BIỆT: "Quay đầu từ từ" (Smooth interpolation)
 * Để robot trông đáng yêu và tự nhiên, khi đổi hướng nhìn, cổ robot sẽ quay
 * từ từ từng chút một chứ không giật mạnh một phát sang bên. Như vậy robot sẽ không bị mỏi cổ!
 */

#ifndef ROBOT_SERVO_H
#define ROBOT_SERVO_H

#include "robot_state.h"
#include <ESP32Servo.h>  // Thư viện giúp điều khiển động cơ quay cổ Servo trên ESP32

/**
 * @class RobotServo
 * @brief Điều khiển chiếc cổ của robot quay theo từng cảm xúc.
 */
class RobotServo {
 private:
  // ===================================================
  // PHẦN CỨNG
  // ===================================================
  Servo myServo;   // Động cơ Servo thực tế
  int servoPin;    // Chân GPIO cắm dây tín hiệu của cổ robot

  // ===================================================
  // TRẠNG THÁI GÓC QUAY
  // ===================================================
  float currentAngle = 90.0;    // Góc hiện tại của cổ robot (bắt đầu bằng nhìn thẳng 90°)
  float targetAngle  = 90.0;    // Góc đích mà robot muốn nhìn tới
  unsigned long lastUpdate = 0; // Lưu thời gian lần cuối robot quay cổ (ms)

  RobotState currentState = NORMAL_HAPPY; // Cảm xúc hiện tại của robot

  // Hẹn giờ để thỉnh thoảng lúc rảnh rỗi robot tự quay cổ nhìn quanh cho đỡ chán
  unsigned long lastNormalHappyMove = 0;

  // ===================================================
  // HÀNH ĐỘNG LẮC ĐẦU NGHỊCH NGỢM (Gentle Shake)
  // ===================================================
  bool isShaking = false;           // Có đang lắc đầu không?
  unsigned long shakeEndTime = 0;   // Đến mấy giờ thì thôi không lắc đầu nữa?

 public:
  /**
   * @brief Hàm lắp ráp cổ robot vào chân GPIO thích hợp.
   */
  RobotServo(int pin) : servoPin(pin) {}

  /**
   * @brief Bắt đầu lắc lắc đầu trong 1 giây (như khi được khen ngoan).
   * Cổ sẽ quay nhanh qua lại giữa 80° và 100°.
   */
  void triggerGentleShake() {
    isShaking    = true;
    shakeEndTime = millis() + 1000; // Lắc trong 1 giây
    targetAngle  = 80.0;            // Bắt đầu nghiêng sang trái
  }

  /**
   * @brief Đánh thức cổ robot dậy. Gọi một lần lúc bắt đầu cắm điện.
   */
  void begin() {
    // ESP32 cần chia sẻ năng lượng timer cho Servo
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);

    // Cài đặt tần số hoạt động chuẩn của động cơ Servo
    myServo.setPeriodHertz(50);
    // Cắm cổ vào cổng tín hiệu với độ dài xung điện cực đại và cực tiểu
    myServo.attach(servoPin, 500, 2400);
    myServo.write((int)currentAngle); // Cho robot nhìn thẳng đằng trước

    lastNormalHappyMove = millis();
  }

  /**
   * @brief Thay đổi hướng quay của cổ tùy theo cảm xúc mới của robot.
   */
  void setState(RobotState state) {
    if (currentState == state) return; // Nếu cảm xúc không đổi thì giữ nguyên hướng cổ

    currentState = state;

    // Tùy theo tâm trạng mới, cổ sẽ chọn hướng quay thích hợp
    switch (currentState) {
      case DANGER_FIRE:
        // Hốt hoảng vì CHÁY! Quay đầu liên tục qua lại từ 60° sang 120° để tìm lối thoát!
        targetAngle = 60.0;
        break;

      case DANGER_HUMID:
        // Ướt quá! Nghiêng đầu hẳn sang một bên (60°) để tránh chỗ ẩm ướt.
        targetAngle = 60.0;
        break;

      case WARNING_HOT:
        // Nóng bức mệt mỏi: Quay đầu uể oải qua lại chậm chạp.
        targetAngle = 60.0;
        break;

      case WARNING_COLD:
        // Lạnh run cầm cập: Rung cổ cực nhanh ở giữa góc 85° và 95°.
        targetAngle = 85.0;
        break;

      case WARNING_DARK:
      case SLEEP_MODE:
      case NORMAL_HAPPY:
        // Yên tâm nhìn thẳng (90°).
        targetAngle = 90.0;
        if (currentState == NORMAL_HAPPY) {
          lastNormalHappyMove = millis(); // Bắt đầu hẹn giờ tự quay cổ
        }
        break;

      case DANCE_MODE:
        // Đang nhảy múa vui vẻ: Quay cổ nhanh qua lại thật rộng từ 60° đến 120°.
        targetAngle = 60.0;
        break;
    }
  }

  /**
   * @brief Điều khiển cổ di chuyển mượt mà. Hàm này chạy liên tục trong loop().
   */
  void update() {
    unsigned long now = millis();
    float step = 0.0;            // Mỗi bước cổ sẽ quay thêm bao nhiêu độ
    unsigned long interval = 20; // Cổ di chuyển bước tiếp theo sau bao nhiêu mili-giây (nhanh hay chậm)

    // --- Ưu tiên lắc đầu cưng nựng trước ---
    if (isShaking) {
      if (now >= shakeEndTime) {
        isShaking = false;
        setState(currentState); // Lắc xong rồi thì trở về hướng nhìn của cảm xúc cũ
      } else {
        // Lắc nhanh qua lại: quay 6° mỗi lần đổi hướng sau 25ms
        interval = 25;
        step     = 6.0;
        if (abs(currentAngle - targetAngle) < 1.0) {
          targetAngle = (targetAngle == 80.0) ? 100.0 : 80.0;
        }
      }
    }
    // --- Nếu không lắc đầu cưng nựng: Quay đầu theo tâm trạng ---
    else {
      switch (currentState) {
        case DANGER_FIRE:
          // Quay đầu cứu hỏa cực nhanh: quay 6 độ sau mỗi 10ms
          interval = 10;
          step     = 6.0;
          if (abs(currentAngle - targetAngle) < 1.0) {
            targetAngle = (targetAngle == 60.0) ? 120.0 : 60.0;
          }
          break;
 
        case DANGER_HUMID:
          // Nghiêng đầu tránh ẩm từ từ: quay 3 độ sau mỗi 15ms
          interval = 15;
          step     = 3.0;
          targetAngle = 60.0;
          break;
 
        case WARNING_HOT:
          // Nóng quá mệt mỏi: lắc cổ rất chậm (chỉ quay 1 độ sau mỗi 50ms)
          interval = 50;
          step     = 1.0;
          if (abs(currentAngle - targetAngle) < 1.0) {
            targetAngle = (targetAngle == 60.0) ? 120.0 : 60.0;
          }
          break;
 
        case WARNING_COLD:
          // Lạnh run rẩy: rung cực nhanh giữa 85° và 95° (quay hẳn 10 độ sau mỗi 30ms)
          interval = 30;
          step     = 10.0;
          if (abs(currentAngle - targetAngle) < 1.0) {
            targetAngle = (targetAngle == 85.0) ? 95.0 : 85.0;
          }
          break;
 
        case WARNING_DARK:
        case SLEEP_MODE:
          // Đứng yên ở giữa: quay chậm rãi về 90°
          interval    = 20;
          step        = 2.0;
          targetAngle = 90.0;
          break;
 
        case NORMAL_HAPPY:
          // Lúc vui vẻ: Đứng yên, thỉnh thoảng cứ sau 3 phút (180000ms) lại tự quay cổ ngó nghiêng ngẫu nhiên
          interval = 30;
          step     = 1.0;
          if (now - lastNormalHappyMove >= 180000) {
            lastNormalHappyMove = now;
            int angles[] = {60, 75, 90, 105, 120};
            targetAngle  = angles[random(0, 5)]; // Chọn bừa 1 trong 5 hướng ngắm
          }
          break;
 
        case DANCE_MODE:
          // Đang nhảy: quay đầu rộng và nhanh nhịp nhàng (bước quay 5 độ sau mỗi 12ms)
          interval = 12;
          step     = 5.0;
          if (abs(currentAngle - targetAngle) < 1.0) {
            targetAngle = (targetAngle == 60.0) ? 120.0 : 60.0;
          }
          break;
      }
    }
 
    // --- Tính toán bước quay cổ từ góc cũ sang góc mới ---
    if (now - lastUpdate >= interval) {
      lastUpdate = now;
 
      if (currentAngle < targetAngle) {
        currentAngle += step;
        if (currentAngle > targetAngle) currentAngle = targetAngle; // Đã quay tới nơi
      } else if (currentAngle > targetAngle) {
        currentAngle -= step;
        if (currentAngle < targetAngle) currentAngle = targetAngle; // Đã quay tới nơi
      }
 
      // Ra lệnh cho động cơ cổ quay thực tế
      myServo.write((int)currentAngle);
    }
  }
 
  /** @return Góc quay cổ hiện tại (độ) */
  int getAngle() const { return (int)currentAngle; }
};
 
#endif  // ROBOT_SERVO_H
