/**
 * @file robot_servo.h
 * @brief Điều khiển ĐỘNG CƠ SERVO - giúp robot quay đầu.
 *
 * Robot có một động cơ servo SG90 gắn ở đầu. Servo này quay
 * theo các góc khác nhau để biểu đạt cảm xúc:
 *  - Góc 0°  : Quay hết sang trái
 *  - Góc 90° : Thẳng giữa (trung tâm)
 *  - Góc 180°: Quay hết sang phải
 *
 * Phương pháp chuyển động "mượt mà" (smooth interpolation):
 * Thay vì nhảy ngay đến góc đích, servo sẽ di chuyển từng bước nhỏ
 * về phía đích. Giống như khi bạn từ từ quay đầu chứ không
 * đột ngột giật sang bên.
 *
 * PHẦN CỨNG: Servo SG90 kết nối vào GPIO 14 (SERVO_PIN)
 */

#ifndef ROBOT_SERVO_H
#define ROBOT_SERVO_H

#include "robot_state.h"
#include <ESP32Servo.h>  // Thư viện điều khiển servo cho ESP32

/**
 * @class RobotServo
 * @brief Điều khiển servo quay đầu robot theo từng trạng thái.
 */
class RobotServo {
 private:
  // ===================================================
  // PHẦN CỨNG
  // ===================================================
  Servo myServo;   // Đối tượng servo
  int servoPin;    // Chân GPIO kết nối servo

  // ===================================================
  // TRẠNG THÁI CHUYỂN ĐỘNG
  // ===================================================
  float currentAngle = 90.0;  // Góc hiện tại của servo (°)
  float targetAngle  = 90.0;  // Góc mục tiêu cần đạt đến (°)
  unsigned long lastUpdate = 0; // Thời điểm cập nhật servo lần cuối (ms)

  RobotState currentState = NORMAL_HAPPY; // Trạng thái robot hiện tại

  // Dùng để đầu robot di chuyển ngẫu nhiên lúc bình thường
  unsigned long lastNormalHappyMove = 0;

  // ===================================================
  // CHỨC NĂNG LẮC ĐẦU (Gentle Shake)
  // ===================================================
  bool isShaking = false;           // Đang lắc đầu không?
  unsigned long shakeEndTime = 0;   // Khi nào kết thúc lắc đầu?

 public:
  /**
   * @brief Hàm khởi tạo.
   * @param pin Chân GPIO kết nối servo
   */
  RobotServo(int pin) : servoPin(pin) {}

  /**
   * @brief Kích hoạt lắc đầu nhẹ trong 1 giây (khi người dùng chạm 2 lần).
   * Servo sẽ qua lại giữa 80° và 100° để tạo cảm giác lắc đầu.
   */
  void triggerGentleShake() {
    isShaking    = true;
    shakeEndTime = millis() + 1000; // Lắc trong 1 giây
    targetAngle  = 80.0;            // Bắt đầu từ phía trái
  }

  /**
   * @brief Khởi động servo. Gọi một lần trong setup().
   * Phân bổ timer PWM của ESP32 và gắn servo vào chân GPIO.
   */
  void begin() {
    // ESP32 cần cấp phát timer PWM trước khi dùng servo
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);

    // Cấu hình servo: tần số 50Hz, xung 500-2400 microseconds
    myServo.setPeriodHertz(50);
    myServo.attach(servoPin, 500, 2400);
    myServo.write((int)currentAngle); // Đặt vị trí ban đầu (90° = thẳng giữa)

    lastNormalHappyMove = millis();
  }

  /**
   * @brief Cập nhật trạng thái servo khi robot thay đổi trạng thái.
   * Mỗi trạng thái sẽ thiết lập góc mục tiêu (targetAngle) khác nhau.
   * @param state Trạng thái mới của robot
   */
  void setState(RobotState state) {
    if (currentState == state) return; // Không làm gì nếu trạng thái không đổi

    currentState = state;

    // Đặt góc đích ban đầu cho từng trạng thái
    switch (currentState) {
      case DANGER_FIRE:
        // Lắc đầu nhanh qua lại - bắt đầu từ 60°
        targetAngle = 60.0;
        break;

      case DANGER_HUMID:
        // Cúi đầu về một phía (tránh ra xa nguồn ẩm giả tưởng)
        targetAngle = 60.0;
        break;

      case WARNING_HOT:
        // Lắc đầu chậm (đang mệt vì nóng) - bắt đầu từ 60°
        targetAngle = 60.0;
        break;

      case WARNING_COLD:
        // Run rẩy qua lại nhanh giữa 85° và 95°
        targetAngle = 85.0;
        break;

      case WARNING_DARK:
      case SLEEP_MODE:
      case NORMAL_HAPPY:
        // Về trung tâm (90°)
        targetAngle = 90.0;
        if (currentState == NORMAL_HAPPY) {
          lastNormalHappyMove = millis(); // Reset timer đầu ngẫu nhiên
        }
        break;

      case DANCE_MODE:
        // Lắc đầu nhanh và rộng hơn - bắt đầu từ 60°
        targetAngle = 60.0;
        break;
    }
  }

  /**
   * @brief Cập nhật chuyển động servo. Gọi liên tục trong loop().
   *
   * Hàm này thực hiện "smooth interpolation":
   * Mỗi lần gọi, servo chỉ di chuyển một bước nhỏ về phía góc mục tiêu.
   * Điều này tạo ra chuyển động mượt mà thay vì nhảy cóc.
   */
  void update() {
    unsigned long now = millis();
    float step = 0.0;           // Số độ di chuyển mỗi bước
    unsigned long interval = 20; // Thời gian giữa các bước cập nhật (ms)

    // --- Chế độ lắc đầu (ưu tiên cao nhất) ---
    if (isShaking) {
      if (now >= shakeEndTime) {
        // Hết thời gian lắc -> Khôi phục trạng thái bình thường
        isShaking = false;
        setState(currentState); // Reset về góc mặc định của trạng thái
      } else {
        // Đang lắc: qua lại giữa 80° và 100° với bước 6°
        interval = 25;
        step     = 6.0;
        if (abs(currentAngle - targetAngle) < 1.0) {
          targetAngle = (targetAngle == 80.0) ? 100.0 : 80.0;
        }
      }
    }
    // --- Chế độ bình thường: chuyển động theo trạng thái ---
    else {
      switch (currentState) {
        case DANGER_FIRE:
          // Lắc đầu rất nhanh (cấp cứu!): bước 6°, cập nhật mỗi 10ms
          interval = 10;
          step     = 6.0;
          if (abs(currentAngle - targetAngle) < 1.0) {
            targetAngle = (targetAngle == 60.0) ? 120.0 : 60.0;
          }
          break;

        case DANGER_HUMID:
          // Từ từ nghiêng về một phía: bước 3°
          interval = 15;
          step     = 3.0;
          targetAngle = 60.0;
          break;

        case WARNING_HOT:
          // Lắc đầu chậm (mệt mỏi vì nóng): bước 1°, cập nhật mỗi 50ms
          interval = 50;
          step     = 1.0;
          if (abs(currentAngle - targetAngle) < 1.0) {
            targetAngle = (targetAngle == 60.0) ? 120.0 : 60.0;
          }
          break;

        case WARNING_COLD:
          // Run rẩy: dao động nhỏ nhanh giữa 85° và 95°
          interval = 30;
          step     = 10.0;
          if (abs(currentAngle - targetAngle) < 1.0) {
            targetAngle = (targetAngle == 85.0) ? 95.0 : 85.0;
          }
          break;

        case WARNING_DARK:
        case SLEEP_MODE:
          // Đứng yên ở giữa (90°)
          interval    = 20;
          step        = 2.0;
          targetAngle = 90.0;
          break;

        case NORMAL_HAPPY:
          // Di chuyển đầu ngẫu nhiên nhẹ nhàng mỗi 3 phút
          interval = 30;
          step     = 1.0;
          if (now - lastNormalHappyMove >= 180000) { // 180000ms = 3 phút
            lastNormalHappyMove = now;
            // Chọn ngẫu nhiên một trong 5 góc: 60, 75, 90, 105, 120
            int angles[] = {60, 75, 90, 105, 120};
            targetAngle  = angles[random(0, 5)];
          }
          break;

        case DANCE_MODE:
          // Lắc đầu nhanh và rộng (đang vui nhảy múa!)
          interval = 12;
          step     = 5.0;
          if (abs(currentAngle - targetAngle) < 1.0) {
            targetAngle = (targetAngle == 60.0) ? 120.0 : 60.0;
          }
          break;
      }
    }

    // --- Di chuyển servo từng bước về phía góc mục tiêu ---
    if (now - lastUpdate >= interval) {
      lastUpdate = now;

      if (currentAngle < targetAngle) {
        currentAngle += step;
        if (currentAngle > targetAngle) currentAngle = targetAngle; // Không vượt quá
      } else if (currentAngle > targetAngle) {
        currentAngle -= step;
        if (currentAngle < targetAngle) currentAngle = targetAngle; // Không vượt quá
      }

      // Ghi góc thực tế ra servo
      myServo.write((int)currentAngle);
    }
  }

  /** @return Góc hiện tại của servo (°) */
  int getAngle() const { return (int)currentAngle; }
};

#endif  // ROBOT_SERVO_H
