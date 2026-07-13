/**
 * @file robot_state.h
 * @brief Định nghĩa các TRẠNG THÁI hoạt động của robot thú cưng.
 *
 * Robot có nhiều trạng thái khác nhau tuỳ vào điều kiện môi trường
 * hoặc tương tác của người dùng. Mỗi trạng thái sẽ ảnh hưởng đến:
 *  - Biểu cảm khuôn mặt trên màn hình OLED
 *  - Màu sắc và hiệu ứng của vòng đèn LED
 *  - Âm thanh của còi (Buzzer)
 *  - Chuyển động của đầu robot (Servo)
 */

// Tránh khai báo file nhiều lần (include guard)
#ifndef ROBOT_STATE_H
#define ROBOT_STATE_H

/**
 * @brief Danh sách tất cả các trạng thái của robot.
 *
 * Enum giúp chúng ta dùng tên dễ đọc thay vì số thứ tự (0, 1, 2...).
 * Ví dụ: thay vì viết "trạng thái 3" ta viết DANGER_FIRE cho dễ hiểu.
 */
enum RobotState {
  DANGER_FIRE,   // Nguy hiểm: Nhiệt độ quá cao (>42°C) -> Báo động cháy!
  DANGER_HUMID,  // Nguy hiểm: Độ ẩm quá cao (>85%) -> Báo động ẩm ướt!
  WARNING_HOT,   // Cảnh báo: Nóng (>35°C) -> Khó chịu, cần chú ý
  WARNING_COLD,  // Cảnh báo: Lạnh (<18°C và ẩm <36%) -> Run rẩy
  WARNING_DARK,  // Cảnh báo: Thiếu sáng liên tục hơn 10 phút
  SLEEP_MODE,    // Chế độ ngủ: Ánh sáng quá tối (<50 lux) -> Ngủ ngon!
  NORMAL_HAPPY,  // Bình thường: Môi trường lý tưởng -> Vui vẻ, hạnh phúc
  DANCE_MODE     // Nhảy múa: Được kích hoạt bởi người dùng
};

#endif  // ROBOT_STATE_H
