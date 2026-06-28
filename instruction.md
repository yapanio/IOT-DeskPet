# KỊCH BẢN VÀ LOGIC ĐIỀU KHIỂN ROBOT GIÁM SÁT MÔI TRƯỜNG

Tài liệu này định nghĩa cấu hình chân linh kiện, các nhóm trạng thái môi trường, điều kiện kích hoạt và logic lập trình (không sử dụng hàm hoãn - `delay()`) cho robot để đảm bảo tính thời gian thực và không xung đột hệ thống.

---

## I. CẤU HÌNH SƠ ĐỒ CHÂN LINH KIỆN (PINOUT)

Dưới đây là danh sách kết nối các linh kiện phần cứng với vi điều khiển:

| STT | Tên Linh Kiện | Loại Giao Tiếp / Tín Hiệu | Chân Kết Nối (Pin) | Ghi Chú |
|:---:|:---|:---|:---:|:---|
| 1 | **Cảm biến nhiệt độ, độ ẩm DHT11** | Digital Input | **Pin 19** | Đo $T$ và $H$ môi trường |
| 2 | **Cảm biến ánh sáng BH1750** | I2C (SDA/SCL) | **Trục Bus I2C** | Đo cường độ sáng (lux) |
| 3 | **Màn hình OLED (4 pin)** | I2C (SDA/SCL) | **Trục Bus I2C** | Hiển thị biểu cảm khuôn mặt |
| 4 | **Đèn LED NeoPixel WS2812 (12 LED)** | Digital Output (Data) | **Pin 4** | Hiển thị hiệu ứng màu sắc RGB |
| 5 | **Servo SG90** | PWM Output | **Pin 14** | Điều khiển khớp đầu chuyển động |
| 6 | **Còi thụ động (Passive Buzzer)** | PWM / Digital Output | **Pin 18** | Phát âm thanh cảnh báo/bíp |
| 7 | **Cảm biến chạm Touch TTP223** | Digital Input | **Pin 15** | Tương tác chạm (nếu cần mở rộng) |

---

## II. CÁC NHÓM TRẠNG THÁI MÔI TRƯỜNG

### 1. Nhóm Trạng Thái Tiêu Chuẩn (Nơi làm việc lý tưởng)
*Xuất hiện khi tất cả các chỉ số đều nằm trong ngưỡng an toàn và thoải mái cho con người.*

#### Trường hợp 1.1: Hoàn hảo hoàn toàn (Ban ngày)
* **Điều kiện:** Nhiệt độ ($22^\circ\text{C} \le T \le 26^\circ\text{C}$) **AND** Độ ẩm ($40\% \le H \le 65\%$) **AND** Ánh sáng ($> 300\text{ lux}$).
* **Robot phản ứng:**
  * **Màn hình:** Mắt tròn xoe vui vẻ hoặc chớp mắt nhẹ.
  * **Đèn LED:** Màu Xanh lá sáng hiệu ứng "nhịp thở" (breathing).
  * **Khớp đầu:** Thỉnh thoảng xoay trái/phải $15^\circ$ ngẫu nhiên sau mỗi 2-3 phút.
  * **Còi:** Tắt.

#### Trường hợp 1.2: Thư giãn / Chế độ ban đêm (Ban đêm)
* **Điều kiện:** Nhiệt độ và Độ ẩm lý tưởng **AND** Ánh sáng tối ($< 50\text{ lux}$).
* **Robot phản ứng:**
  * **Màn hình:** Chuyển sang mắt nhắm `(- _ -)`.
  * **Đèn LED:** Chuyển sang màu Xanh dương mờ hoặc Tím (giảm độ sáng xuống 10%).
  * **Khớp đầu:** Quay về chính giữa ($90^\circ$) và đứng yên.
  * **Còi:** Tắt.

---

### 2. Nhóm Cảnh Báo Môi Trường (Cần điều chỉnh không gian)
*Nhắc nhở người dùng tối ưu lại phòng làm việc để bảo vệ sức khỏe.*

#### Trường hợp 2.1: Quá nóng hoặc Ngột ngạt (Hiệu ứng nhà kính)
* **Điều kiện:** Nhiệt độ thực tế $T > 30^\circ\text{C}$ **OR** Nhiệt độ cảm nhận $T_{\text{feel}} > 33^\circ\text{C}$. Ánh sáng bình thường.
* **Robot phản ứng:**
  * **Màn hình:** Mắt mệt mỏi, rủ xuống `(~ _ ~)`.
  * **Đèn LED:** Chuyển sang màu Cam.
  * **Khớp đầu:** Xoay chậm sang trái rồi sang phải như đang mệt mỏi, hoặc cúi nhẹ xuống.
  * **Còi:** Kêu 1 tiếng "bíp" ngắn sau mỗi 5 phút để nhắc nhở bật quạt/điều hòa.

#### Trường hợp 2.2: Lạnh và Khô (Phòng điều hòa quá đà)
* **Điều kiện:** Nhiệt độ $T < 18^\circ\text{C}$ **AND** Độ ẩm $H < 35\%$.
* **Robot phản ứng:**
  * **Màn hình:** Mắt run rẩy hoặc mếu.
  * **Đèn LED:** Màu Trắng hoặc Xanh lam nhạt.
  * **Khớp đầu:** Giữ nguyên góc nhưng thỉnh thoảng "rung" nhẹ (xoay nhanh qua lại $5^\circ$ rồi dừng để giả vờ run rẩy).
  * **Còi:** Tắt.

#### Trường hợp 2.3: Thiếu sáng khi đang làm việc (Cảnh báo mỏi mắt)
* **Điều kiện:** Ánh sáng yếu ($50\text{ lux} \le \text{Ánh sáng} \le 150\text{ lux}$) **AND** Thời gian đo được duy trì liên tục quá 10 phút.
* **Robot phản ứng:**
  * **Màn hình:** Mắt nheo lại hoặc hiển thị biểu tượng bóng đèn.
  * **Đèn LED:** Chớp tắt màu Vàng chậm.
  * **Khớp đầu:** Ngước lên trên (hướng nhìn lên).
  * **Còi:** Kêu "bíp... bíp" âm lượng nhỏ mỗi 1 phút để nhắc bật đèn bàn.

---

### 3. Nhóm Cảnh Báo Nguy Hiểm (Cần xử lý ngay)
*Mức độ ưu tiên cao nhất trong code, bỏ qua các trạng thái biểu cảm thông thường.*

#### Trường hợp 3.1: Nguy cơ hỏa hoạn / Chập cháy thiết bị
* **Điều kiện:** Nhiệt độ tăng đột biến hoặc $T > 42^\circ\text{C}$.
* **Robot phản ứng:**
  * **Màn hình:** Hiển thị dấu chấm than lớn hoặc mắt chữ X hoảng loạn `(X _ X)`.
  * **Đèn LED:** Đỏ chớp nháy liên tục với tần số cao (Strobe light).
  * **Khớp đầu:** Xoay liên tục từ cực trái ($0^\circ$) sang cực phải ($180^\circ$) với tốc độ tối đa.
  * **Còi:** Hú liên tục hoặc kêu "bíp bíp bíp" dồn dập.

#### Trường hợp 3.2: Độ ẩm quá cao (Nguy cơ hỏng linh kiện điện tử)
* **Điều kiện:** Độ ẩm $H > 85\%$ *(Ví dụ: máy phun sương chĩa thẳng vào bàn máy tính).*
* **Robot phản ứng:**
  * **Màn hình:** Hiển thị biểu tượng giọt nước lớn hoặc mắt khóc.
  * **Đèn LED:** Màu Đỏ sáng cố định không nhấp nháy.
  * **Đầu robot:** Quay hẳn sang hướng ngược lại với hướng cảm biến bị ẩm ($0^\circ$).
  * **Còi:** Phát âm thanh cảnh báo kéo dài.

---

## III. THAM KHẢO TƯ DUY VIẾT CODE (PSEUDO-CODE)

```cpp
// ==========================================
// ĐỊNH NGHĨA CÁC HẰNG SỐ VÀ NGƯỠNG (THRESHOLDS)
// ==========================================
CONSTANT TEMP_FIRE      = 42     // Ngưỡng cháy
CONSTANT HUMID_MAX      = 85     // Ngưỡng ẩm nguy hại cho đồ điện tử
CONSTANT TEMP_HOT       = 30     // Ngưỡng nóng
CONSTANT TEMP_FEEL_HOT  = 33     // Ngưỡng cảm nhận nóng
CONSTANT TEMP_COLD      = 18     // Ngưỡng lạnh
CONSTANT HUMID_LOW      = 35     // Ngưỡng khô
CONSTANT LIGHT_DARK     = 50     // Ngưỡng tối hẳn (Chế độ ngủ)
CONSTANT LIGHT_WEAK     = 150    // Ngưỡng thiếu sáng (Cần bật đèn)

// Định nghĩa các trạng thái của Robot
ENUM RobotState {
    DANGER_FIRE,
    DANGER_HUMID,
    WARNING_HOT,
    WARNING_COLD,
    WARNING_DARK,
    SLEEP_MODE,
    NORMAL_HAPPY
}

// Các biến lưu vết thời gian (Non-blocking timing)
float last_time_light_checked = 0
float last_time_buzzer_sounded = 0
float last_time_servo_moved = 0
RobotState current_state = NORMAL_HAPPY

// ==========================================
// VÒNG LẶP CHÍNH (MAIN LOOP)
// ==========================================
FUNCTION loop():
    // 1. ĐỌC DỮ LIỆU TỪ CẢM BIẾN
    float t      = Đọc_nhiệt_độ_C()
    float h      = Đọc_độ_ẩm_phần_trăm()
    float lux    = Đọc_độ_sáng()
    float t_feel = Tính_nhiệt_độ_cảm_nhận(t, h)
    
    // 2. PHÂN TÍCH LOGIC ĐỂ XÁC ĐỊNH TRẠNG THÁI (Ưu tiên từ cao xuống thấp)
    IF (t > TEMP_FIRE) THEN
        current_state = DANGER_FIRE
    ELSE IF (h > HUMID_MAX) THEN
        current_state = DANGER_HUMID
    ELSE IF (t > TEMP_HOT OR t_feel > TEMP_FEEL_HOT) THEN
        current_state = WARNING_HOT
    ELSE IF (t < TEMP_COLD AND h < HUMID_LOW) THEN
        current_state = WARNING_COLD
    ELSE IF (lux < LIGHT_DARK) THEN
        current_state = SLEEP_MODE
    ELSE IF (lux < LIGHT_WEAK) THEN
        // Kiểm tra xem có bị thiếu sáng liên tục 10 phút (600,000 ms) không
        IF (Thời_gian_hiện_tại() - last_time_light_checked > 600000) THEN
            current_state = WARNING_DARK
        END IF
    ELSE
        current_state = NORMAL_HAPPY
        last_time_light_checked = Thời_gian_hiện_tại() // Reset bộ đếm thời gian ánh sáng
    END IF

    // 3. THỰC THI HÀNH VI DỰA TRÊN TRẠNG THÁI (Không dùng delay)
    SWITCH (current_state):
    
        CASE DANGER_FIRE:
            Hiển_thị_màn_hình("BIỂU CẢM: X _ X (Hoảng loạn)")
            Chớp_tắt_LED_RGB(MÀU_ĐỎ, Tốc_độ = Rất_nhanh)
            Phát_âm_thanh_còi(Tần_số = Cao, Kiểu = Liên_tục_dồn_dập)
            Xoay_servo_qua_lại_không_chặn(Từ_góc = 0, Đến_góc = 180, Tốc_độ = Nhanh_nhất)
            BREAK

        CASE DANGER_HUMID:
            Hiển_thị_màn_hình("BIỂU CẢM: Mắt khóc / Giọt nước")
            Bật_LED_RGB(MÀU_ĐỎ, Độ_sáng = 100%)
            Phát_âm_thanh_còi(Tần_số = Trung_bình, Kiểu = Kéo_dài)
            Quay_servo_về_góc(0) // Tránh hướng phun sương
            BREAK

        CASE WARNING_HOT:
            Hiển_thị_màn_hình("BIỂU CẢM: ~ _ ~ (Mệt mỏi)")
            Bật_LED_RGB(MÀU_CAM, Độ_sáng = 70%)
            Xoay_servo_qua_lại_chậm(Từ_góc = 60, Đến_góc = 120, Tốc_độ = Chậm)
            // Nhắc nhở bằng tiếng bíp ngắn mỗi 5 phút (300,000 ms)
            IF (Thời_gian_hiện_tại() - last_time_buzzer_sounded > 300000) THEN
                Phát_âm_thanh_còi(1 tiếng bíp ngắn)
                last_time_buzzer_sounded = Thời_gian_hiện_tại()
            END IF
            BREAK

        CASE WARNING_COLD:
            Hiển_thị_màn_hình("BIỂU CẢM: Mếu / Răng lập cập")
            Bật_LED_RGB(MÀU_XANH_LƠ_HOẶC_TRẮNG)
            Tắt_còi()
            Rung_lắc_servo_nhẹ(Góc_tâm = 90, Biên_độ = 5)
            BREAK

        CASE WARNING_DARK:
            Hiển_thị_màn_hình("BIỂU CẢM: Nheo mắt / Hình bóng đèn")
            Chớp_tắt_LED_RGB(MÀU_VÀNG, Tốc_độ = Chậm)
            Quay_servo_ngước_lên(Góc = 120)
            // Nhắc nhở bật đèn mỗi 1 phút (60,000 ms)
            IF (Thời_gian_hiện_tại() - last_time_buzzer_sounded > 60000) THEN
                Phát_âm_thanh_còi(2 tiếng bíp nhỏ)
                last_time_buzzer_sounded = Thời_gian_hiện_tại()
            END IF
            BREAK

        CASE SLEEP_MODE:
            Hiển_thị_màn_hình("BIỂU CẢM: - _ - (Mắt nhắm ngủ)")
            Bật_LED_RGB(MÀU_TÍM_HOẶC_XANH_DƯƠNG, Độ_sáng = 10%)
            Tắt_còi()
            Quay_servo_về_góc(90)
            BREAK

        CASE NORMAL_HAPPY:
            Hiển_thị_màn_hình("BIỂU CẢM: ^ _ ^ (Vui vẻ)")
            Hiệu_ứng_LED_thở_Breathing(MÀU_XANH_LÁ)
            Tắt_còi()
            // Sau mỗi 3 phút (180,000 ms), tự động xoay ngẫu nhiên nhìn quanh
            IF (Thời_gian_hiện_tại() - last_time_servo_moved > 180000) THEN
                int goc_ngau_nhien = Chọn_ngẫu_nhiên_từ_dãy(60, 75, 90, 105, 120)
                Quay_servo_về_góc(goc_ngau_nhien)
                last_time_servo_moved = Thời_gian_hiện_tại()
            END IF
            BREAK
            
    END SWITCH

    // 4. CẬP NHẬT TRẠNG THÁI PHẦN CỨNG (Hàm update chạy ngầm không chặn)
    Gọi_hàm_update_từng_linh_kiện()