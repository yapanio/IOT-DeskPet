/**
 * @file robot_state.h
 * @brief Định nghĩa các CẢM XÚC (Trạng thái) của chú robot thú cưng.
 *
 * Cũng giống như chúng ta, chú robot này cũng biết vui, buồn, sợ hãi hay buồn ngủ đấy!
 * Tùy vào thời tiết xung quanh hoặc cách chúng ta chơi đùa, robot sẽ có các cảm xúc khác nhau:
 *  - Màn hình OLED sẽ thay đổi hình dáng đôi mắt.
 *  - Vòng đèn LED trên người sẽ đổi màu sắc.
 *  - Còi Buzzer sẽ kêu bíp bíp báo động hoặc hát nhạc vui tươi.
 *  - Cổ của robot (động cơ servo) sẽ quay đầu linh hoạt.
 */

#ifndef ROBOT_STATE_H
#define ROBOT_STATE_H

/**
 * @brief Bảng tâm trạng của robot.
 *
 * Enum là cách chúng ta đặt tên cho các cảm xúc để máy tính dễ hiểu.
 * Thay vì dùng các con số khô khan (0, 1, 2...), ta dùng những tên gọi đáng yêu dưới đây:
 */
enum RobotState {
  DANGER_FIRE,   // 1. Quá nóng (>42°C): Robot hoảng hốt kêu "Cháy! Cháy!" (Đèn chớp đỏ, còi hú siren).
  DANGER_HUMID,  // 2. Ướt sũng (>85% độ ẩm): Robot sợ bị ẩm ướt hỏng mạch (Đèn đỏ đặc, còi kêu liên tục).
  WARNING_HOT,   // 3. Trời nóng nực (>35°C): Robot mệt mỏi khó chịu (Đèn màu cam, tự hát Despacito).
  WARNING_COLD,  // 4. Trời lạnh giá (<18°C): Robot run bần bật (Đèn màu xanh cyan nhạt, tự hát Jingle Bells).
  WARNING_DARK,  // 5. Phòng tối om quá 10 phút: Robot thấy cô đơn (Đèn nháy vàng, kêu bíp đôi định kỳ).
  SLEEP_MODE,    // 6. Đi ngủ: Khi trời tối hẳn (<50 lux), robot sẽ nhắm mắt ngủ khò khò (Đèn tím mờ nhè nhẹ).
  NORMAL_HAPPY,  // 7. Vui vẻ hạnh phúc: Thời tiết mát mẻ lý tưởng, robot nhìn quanh và chớp mắt cười yêu đời.
  DANCE_MODE     // 8. Nhảy múa: Khi được con chạm tay 3 lần hoặc bấm nút trên điện thoại, robot sẽ xoay đầu nhảy múa cực vui!
};

#endif  // ROBOT_STATE_H
