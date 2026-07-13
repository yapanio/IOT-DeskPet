/**
 * @file Project.ino
 * @brief Điểm khởi đầu (Entry Point) của chương trình robot thú cưng IOT DeskPet.
 *
 * File này là file chính của Arduino. Arduino IDE sẽ tự động
 * gọi setup() một lần khi khởi động, và gọi loop() liên tục sau đó.
 *
 * Toàn bộ logic phức tạp được đóng gói trong class Controller (controller.h).
 * File này chỉ:
 *  1. Khai báo đối tượng robotController
 *  2. Đăng ký các hàm callback từ Blynk (BLYNK_WRITE)
 *  3. Gọi begin() và update() cho controller
 *
 * BLYNK VIRTUAL PINS:
 *  V5 <- Nhận ID bài nhạc từ ứng dụng Blynk (1=Mario, 2=Despacito, 3=Jingle Bells)
 *  V6 <- Nhận lệnh chạm ảo (1=Nhấn, 0=Thả)
 *
 * =====================================
 * NHÓM THỰC HIỆN:
 *  - Nguyễn Huy Nhật  (HE204465)
 *  - Lưu Chí Kiên     (HE204365)
 *  - Phạm Công Hùng   (HEXXXXXX)
 * TRƯỜNG: FPT University
 * MÔN HỌC: IOT102 - Internet of Things
 * =====================================
 */

// Bao gồm toàn bộ logic robot từ file controller.h
#include "controller.h"

// ===================================================
// KHAI BÁO ĐỐI TƯỢNG ROBOT CONTROLLER (Toàn cục)
// ===================================================
// Đây là "bộ não" của robot - chứa tất cả các module con
Controller robotController;

// ===================================================
// BLYNK CALLBACKS - Hàm được gọi tự động khi nhận dữ liệu từ Blynk
// ===================================================

/**
 * @brief Nhận lệnh phát nhạc từ Blynk Virtual Pin V5.
 *
 * Khi người dùng chọn nhạc trên ứng dụng Blynk điện thoại,
 * Blynk sẽ tự động gọi hàm này với giá trị songId tương ứng.
 *
 * Giá trị songId:
 *  1 -> Phát bài Super Mario Theme
 *  2 -> Phát bài Despacito
 *  3 -> Phát bài Jingle Bells
 *  0 -> Dừng nhạc và thoát Dance Mode
 */
BLYNK_WRITE(V5) {
  int songId = param.asInt(); // Đọc giá trị từ Blynk
  robotController.playSongBlynk(songId);
}

/**
 * @brief Nhận lệnh chạm ảo từ Blynk Virtual Pin V6.
 *
 * Cho phép người dùng điều khiển robot qua điện thoại
 * như thể đang chạm trực tiếp vào cảm biến vật lý.
 *
 * Giá trị:
 *  1 -> Đang nhấn (pressed)
 *  0 -> Đã thả (released)
 */
BLYNK_WRITE(V6) {
  int pressed = param.asInt();
  robotController.setBlynkTouch(pressed == 1);
}

// ===================================================
// HÀM SETUP - Chạy 1 LẦN khi khởi động
// ===================================================
/**
 * @brief Arduino gọi hàm này một lần duy nhất sau khi cấp nguồn hoặc reset.
 * Khởi động toàn bộ hệ thống robot.
 */
void setup() {
  robotController.begin();
}

// ===================================================
// HÀM LOOP - Chạy LIÊN TỤC sau setup()
// ===================================================
/**
 * @brief Arduino gọi hàm này lặp lại liên tục (như một vòng lặp vô hạn).
 * Mọi logic chạy trong vòng lặp đều không được dùng delay() để
 * robot luôn phản hồi nhanh (non-blocking design).
 */
void loop() {
  robotController.update();
}
