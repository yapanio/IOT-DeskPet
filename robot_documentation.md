# TÀI LIỆU VẬN HÀNH VÀ LOGIC ĐIỀU KHIỂN ROBOT GIÁM SÁT MÔI TRƯỜNG

Tài liệu này chi tiết cấu hình phần cứng, các trạng thái hoạt động, logic chuyển đổi trạng thái và hành vi cụ thể của từng linh kiện (Màn hình OLED, Đèn LED NeoPixel, Động cơ Servo, Còi Buzzer) của hệ thống Robot IoT.

---

## I. CẤU HÌNH SƠ ĐỒ CHÂN PHẦN CỨNG (PINOUT)

Tất cả cấu hình chân linh kiện được gom lại tại một nơi duy nhất ở đầu tệp `controller.h` để dễ dàng quản lý và chỉnh sửa:

| Tên Linh Kiện / Tín Hiệu | Định nghĩa chân | Chân Kết Nối (ESP32 Pin) | Ghi Chú |
| :--- | :---: | :---: | :--- |
| **Cảm biến chạm (Touch TTP223)** | `TOUCH_PIN` | **Pin 15** | Digital Input |
| **Cảm biến nhiệt độ, độ ẩm DHT11** | `DHT_PIN` | **Pin 19** | Digital Input |
| **Động cơ Servo SG90** | `SERVO_PIN` | **Pin 14** | PWM Output (Tần số 50Hz) |
| **Đèn LED NeoPixel WS2812B** | `LED_PIN` | **Pin 4** | Digital Output (Data) |
| **Còi thụ động (Passive Buzzer)** | `BUZZER_PIN` | **Pin 18** | PWM / Digital Output |
| **Trục I2C (SDA/SCL)** | Mặc định phần cứng | **SDA / SCL** | Kết nối màn hình OLED và BH1750 |

---

## II. THIẾT KẾ PHẦN MỀM VÀ PHÂN CHIA MODUL (MVC)

Hệ thống được thiết kế theo hướng đối tượng hướng tới sự độc lập của linh kiện (Modular design):
* **`Project.ino`**: Hàm khởi chạy chính (`setup`, `loop`) gọi tuần tự bộ điều khiển trung tâm.
* **`Controller` (`controller.h`)**: Bộ điều khiển trung tâm (Controller). Quản lý chu trình đọc cảm biến, logic chuyển đổi trạng thái, quản lý các tác vụ thời gian thực và tương tác người dùng.
* **`Sensors` (`sensors.h`)**: Lớp quản lý dữ liệu đầu vào. Đọc không chặn (non-blocking) từ cảm biến DHT11 và BH1750.
* **`Actuators` (`actuators.h`)**: Lớp điều khiển đầu ra cho LED NeoPixel (WS2812B) và Còi (Buzzer).
* **`RobotServo` (`robot_servo.h`)**: Lớp điều khiển chuyển động của khớp cổ Robot (Servo SG90) giới hạn góc an toàn.
* **`Emote` (`emote.h`)**: Lớp điều khiển giao diện biểu cảm của mắt Robot (RoboEyes) và hiển thị thông số môi trường trên màn hình OLED.

---

## III. CÁC TRẠNG THÁI HOẠT ĐỘNG (STATES) & LOGIC CHUYỂN ĐỔI

Robot liên tục đánh giá thông tin môi trường để chuyển đổi giữa 7 trạng thái hoạt động dựa trên cây ưu tiên (từ cao xuống thấp):

```mermaid
graph TD
    A[Bắt đầu đo cảm biến] --> B{Nhiệt độ > 42°C?}
    B -- Đúng --> C[DANGER_FIRE]
    B -- Sai --> D{Độ ẩm > 85%?}
    D -- Đúng --> E[DANGER_HUMID]
    D -- Sai --> F{Nhiệt độ > 30°C hoặc chỉ số cảm nhận nhiệt > 33°C?}
    F -- Đúng --> G[WARNING_HOT]
    F -- Sai --> H{Nhiệt độ < 18°C và Độ ẩm < 35%?}
    H -- Đúng --> I[WARNING_COLD]
    H -- Sai --> J{Ánh sáng < 50 lux?}
    J -- Đúng --> K[SLEEP_MODE]
    J -- Sai --> L{Ánh sáng < 150 lux liên tục > 10 phút?}
    L -- Đúng --> M[WARNING_DARK]
    L -- Sai --> N[NORMAL_HAPPY]
```

### Chi tiết logic bộ đếm thời gian ánh sáng yếu (`WARNING_DARK`):
* Khi cường độ sáng rơi vào khoảng nguy cơ ($50\text{ lux} \le \text{Lux} < 150\text{ lux}$), bộ đếm thời gian bắt đầu chạy. Nếu tình trạng này kéo dài liên tục **quá 10 phút (600,000 ms)**, Robot mới chuyển sang trạng thái cảnh báo thiếu sáng `WARNING_DARK`.
* Nếu cường độ sáng phục hồi ($\ge 150\text{ lux}$) hoặc tối hẳn ($< 50\text{ lux}$), bộ đếm thời gian sẽ ngay lập tức được reset để tránh các cảnh báo nhầm không đáng có.

---

## IV. BẢNG CHI TIẾT HÀNH VI CỦA ROBOT TRONG TỪNG TRẠNG THÁI

Khớp đầu Servo của robot được cấu hình ở góc gốc là **90° (nhìn thẳng)**, chỉ có thể xoay trái phải và giới hạn góc tối đa trong khoảng **`[60°, 120°]`** (tức là lệch tối đa $\pm 30^\circ$ so với tâm) để bảo vệ cấu trúc cơ học của robot.

| Trạng thái | Điều kiện kích hoạt | Biểu cảm OLED (Emote) | Đèn LED NeoPixel | Khớp đầu Servo (SG90) | Còi Buzzer (Báo động) |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **`DANGER_FIRE`** <br>*(Nguy cơ hỏa hoạn)* | $T > 42^\circ\text{C}$ | Hiển thị biểu tượng hoảng loạn `X _ X` (Vẽ trực tiếp bằng đường thẳng). | Chớp tắt màu ĐỎ liên tục với tần số cao (mỗi 100ms). | Quay nhanh qua lại liên tục giữa 2 cực hạn `60°` và `120°` (`step = 6.0`, `interval = 10ms`). | Còi hú báo động liên tục, thay đổi tần số (2500Hz và 1800Hz) mỗi 120ms. |
| **`DANGER_HUMID`** <br>*(Độ ẩm nguy hại)* | $H > 85\%$ | Mắt rủ mệt mỏi (`TIRED`), đổ mồ hôi (`Sweat` hoạt động). | Màu ĐỎ sáng tĩnh ở độ sáng tối đa 100%. | Quay hẳn sang góc cực hạn `60°` (tránh hướng có hơi ẩm của máy phun sương) và đứng yên. | Kêu còi cảnh báo kéo dài liên tục ở tần số cố định 1000Hz. |
| **`WARNING_HOT`** <br>*(Môi trường quá nóng)* | $T > 30^\circ\text{C}$ hoặc $T_{\text{feel}} > 33^\circ\text{C}$ | Mắt mệt mỏi (`TIRED`). | Màu CAM sáng tĩnh ở độ sáng trung bình 70%. | Xoay chậm chạp qua lại giữa `60°` và `120°` (`step = 1.0`, `interval = 50ms`). | Phát 1 tiếng bíp ngắn (1200Hz, 150ms) sau mỗi **5 phút (300,000ms)** để nhắc nhở. |
| **`WARNING_COLD`** <br>*(Môi trường lạnh khô)* | $T < 18^\circ\text{C}$ và $H < 35\%$ | Mắt mệt mỏi, chế độ rung mắt run rẩy (`HFlicker`). | Màu XANH LƠ sáng tĩnh (70% độ sáng). | Rung lắc nhẹ giả vờ run rẩy (dao động nhanh giữa `85°` và `95°`, `step = 10.0`, `interval = 30ms`). | Tắt còi. |
| **`WARNING_DARK`** <br>*(Cảnh báo thiếu sáng)*| $50 \le \text{Lux} < 150$ kéo dài quá 10 phút. | Mắt nheo lại giận dữ (`ANGRY`) và hướng nhìn lên phía trên. | Chớp tắt màu VÀNG chậm (mỗi 500ms). | Quay về vị trí chính giữa `90°`. | Phát tiếng bíp đôi (1500Hz, độ dài 80ms, khoảng cách 80ms) sau mỗi **1 phút (60,000ms)**. |
| **`SLEEP_MODE`** <br>*(Chế độ ngủ đêm)* | $\text{Lux} < 50$ | Mắt nhắm hẳn lại ngủ ngon. | Màu TÍM mờ (10% độ sáng) đóng vai trò làm đèn ngủ ban đêm. | Quay về chính giữa `90°` và đứng yên hoàn toàn. | Tắt còi. |
| **`NORMAL_HAPPY`** <br>*(Bình thường vui vẻ)*| Tất cả các chỉ số đều ở ngưỡng an toàn. | Mắt cười (`HAPPY`), thỉnh thoảng chớp mắt tự động. | Hiệu ứng "nhịp thở" (breathing) màu XANH LÁ CÂY (chu kỳ 3000ms). | Thỉnh thoảng nhìn ngó xung quanh: ngẫu nhiên quay về góc `{60, 75, 90, 105, 120}` mỗi **3 phút (180,000ms)**. | Tắt còi. |

---

## V. TƯƠNG TÁC NGƯỜI DÙNG QUA CẢM BIẾN CHẠM (`TOUCH_PIN`)

Robot hỗ trợ giao tiếp tương tác qua cảm biến chạm điện dung TTP223 (sử dụng cơ chế chống rung phím - debounce 1 giây):
* **Khi Robot ở trạng thái Vui vẻ (`NORMAL_HAPPY`)**:
  * Người dùng chạm vào đầu cảm biến -> Robot lập tức còi bíp đôi vui vẻ (2000Hz, 80ms) và kích hoạt hiệu ứng hoạt ảnh mắt cười lớn (`anim_laugh`).
* **Khi Robot ở các trạng thái Cảnh báo / Nguy hiểm**:
  * Người dùng chạm vào -> Robot sẽ hiển thị hoạt ảnh mắt bối rối (`anim_confused`) để biểu thị việc đang không thoải mái do môi trường xấu.

---

## VI. NGUYÊN LÝ LẬP TRÌNH KHÔNG CHẶN (NON-BLOCKING TIMING)

Để đảm bảo khả năng phản hồi thời gian thực, toàn bộ chương trình **không sử dụng bất kỳ hàm hoãn thời gian `delay()` nào**.
Thay vào đó, cơ chế hoạt động song song dựa trên việc so sánh mốc thời gian qua hàm `millis()`:
* **LED Breathing**: Sử dụng hàm lượng giác `sin(millis())` để tính toán cường độ sáng mượt mà theo chu kỳ thời gian thực mà không chặn CPU.
* **Servo Sweeping & Shivering**: Sử dụng các mốc thời gian riêng biệt (`interval` từ 10ms đến 50ms) để di chuyển động cơ từng bước nhỏ, tạo hiệu ứng chuyển động mượt mà trong khi CPU vẫn xử lý được cảm biến và âm thanh.
* **Buzzer Sequence State Machine**: Sử dụng một máy trạng thái (state machine) 4 bước để kiểm soát còi kêu bíp đơn/kêu bíp đôi không chặn luồng chính.
