/**
 * @file Project.ino
 * @brief Đây là ngôi nhà chính của chú Robot thú cưng DeskPet!
 * 
 * Hãy tưởng tượng chú Robot này giống như một chú cún con bằng máy.
 * File này là nơi chú Robot bắt đầu thức dậy và hoạt động suốt cả ngày.
 *
 * Chú Robot có hai việc chính để làm:
 *  1. Thức dậy (setup): Robot sẽ mở mắt, kiểm tra xem chân tay, cảm biến có chạy tốt không.
 *  2. Hoạt động liên tục (loop): Robot cứ lặp đi lặp lại việc kiểm tra môi trường và chơi đùa với chúng ta.
 *
 * Ngoài ra, robot còn có "tai thần kỳ" (Blynk) để nghe lời chúng ta ra lệnh từ xa bằng điện thoại nữa đấy!
 *
 * =====================================
 * CÁC BẠN NHỎ ĐÃ TẠO RA ROBOT:
 *  - Nguyễn Huy Nhật  (HE204465)
 *  - Lưu Chí Kiên     (HE204365)
 *  - Phạm Công Hùng   (HEXXXXXX)
 * Trường đại học FPT
 * =====================================
 */

// Lấy "sách hướng dẫn điều khiển" robot từ file controller.h
#include "controller.h"

// ===================================================
// TẠO RA MỘT CHÚ ROBOT (Đối tượng Robot Controller)
// ===================================================
// Dòng này giống như chúng ta lắp ráp một chú robot hoàn chỉnh.
// Chú robot này tên là "robotController", có đầy đủ mắt, mũi, tai, và còi.
Controller robotController;

// ===================================================
// TAI THẦN KỲ CỦA ROBOT - Nghe lệnh từ điện thoại (Blynk)
// ===================================================

/**
 * @brief Chiếc tai ảo số V5: Nghe xem chúng ta muốn robot hát bài gì.
 *
 * Khi con bấm nút chọn bài hát trên điện thoại:
 *  - Số 1: Robot sẽ nhảy múa theo nhạc của chú thợ sửa ống nước Super Mario!
 *  - Số 2: Robot sẽ nhảy múa theo điệu Despacito sôi động!
 *  - Số 3: Robot sẽ nhảy theo bài Jingle Bells mừng Giáng sinh!
 *  - Số 0: Robot bảo "Mệt rồi, không hát nữa đâu!" và đứng yên.
 */
BLYNK_WRITE(V5) {
  int songId = param.asInt(); // Nhận số bài hát từ điện thoại
  robotController.playSongBlynk(songId); // Ra lệnh cho robot hát bài đó
}

/**
 * @brief Chiếc tai ảo số V6: Nghe xem chúng ta có chạm vào robot từ xa không.
 *
 * Nhấn nút trên điện thoại cũng giống như con chạm tay trực tiếp vào robot:
 *  - Số 1: Cảm giác giống như con đang chạm tay vào robot.
 *  - Số 0: Con đã bỏ tay ra rồi.
 */
BLYNK_WRITE(V6) {
  int pressed = param.asInt(); // Đọc xem nút được nhấn hay thả ra
  robotController.setBlynkTouch(pressed == 1); // Báo cho robot biết để phản hồi
}

// ===================================================
// THỨC DẬY (Hàm setup - Chạy duy nhất 1 lần khi cắm điện)
// ===================================================
/**
 * @brief Giống như buổi sáng con thức dậy và chuẩn bị sách vở.
 * Khi cắm điện vào, robot sẽ chạy hàm này đầu tiên để khởi động màn hình,
 * động cơ cổ, đèn LED và còi báo.
 */
void setup() {
  robotController.begin(); // Gọi lệnh khởi động robot
}

// ===================================================
// HOẠT ĐỘNG LIÊN TỤC (Hàm loop - Chạy lặp đi lặp lại mãi mãi)
// ===================================================
/**
 * @brief Đây là trái tim và bộ não đang hoạt động của robot!
 * Hàm này chạy đi chạy lại siêu nhanh (hàng nghìn lần trong 1 giây).
 * Robot sẽ liên tục nhìn xung quanh, đo nhiệt độ xem phòng có bị nóng không,
 * xem con có chạm vào đầu nó không để nháy mắt hay cười vui vẻ.
 */
void loop() {
  robotController.update(); // Yêu cầu robot cập nhật mọi hành động liên tục
}
