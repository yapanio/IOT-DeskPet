# KỊCH BẢN VÀ LOGIC ĐIỀU KHIỂN ROBOT GIÁM SÁT MÔI TRƯỜNG

Tài liệu này định nghĩa cấu hình chân linh kiện, các nhóm trạng thái môi trường, điều kiện kích hoạt và logic lập trình (không sử dụng hàm hoãn - `delay()`) cho robot để đảm bảo tính thời gian thực và không xung đột hệ thống.

---

## I. CẤU HÌNH SƠ ĐỒ CHÂN LINH KIỆN (PINOUT) & KÊNH TRUYỀN BLYNK

### 1. Cấu hình kết nối phần cứng với ESP32:

| STT | Tên Linh Kiện | Loại Giao Tiếp / Tín Hiệu | Chân Kết Nối (Pin) | Ghi Chú |
|:---:|:---|:---|:---:|:---|
| 1 | **Cảm biến nhiệt độ, độ ẩm DHT11** | Digital Input | **Pin 19** | Đo $T$ và $H$ môi trường |
| 2 | **Cảm biến ánh sáng BH1750** | I2C (SDA/SCL) | **Trục Bus I2C** | Đo cường độ sáng (lux) |
| 3 | **Màn hình OLED (4 pin)** | I2C (SDA/SCL) | **Trục Bus I2C** | Hiển thị biểu cảm khuôn mặt |
| 4 | **Đèn LED NeoPixel WS2812 (12 LED)** | Digital Output (Data) | **Pin 4** | Hiển thị hiệu ứng màu sắc RGB |
| 5 | **Servo SG90** | PWM Output | **Pin 14** | Điều khiển khớp đầu chuyển động |
| 6 | **Còi thụ động (Passive Buzzer)** | PWM / Digital Output | **Pin 18** | Phát âm thanh cảnh báo/bản nhạc |
| 7 | **Cảm biến chạm Touch TTP223** | Digital Input | **Pin 15** | Tương tác chạm (Single/Double/Long press) |

### 2. Cấu hình kênh ảo Blynk (Blynk Virtual Pins):
- **V0 (Output)**: Gửi chỉ số Nhiệt độ ($T$) lên đám mây.
- **V1 (Output)**: Gửi chỉ số Độ ẩm ($H$) lên đám mây.
- **V2 (Output)**: Gửi chỉ số Cường độ sáng (Lux) lên đám mây.
- **V3 (Output)**: Gửi chỉ số Nhiệt độ cảm nhận (Heat Index) lên đám mây.
- **V4 (Output)**: Gửi tên Trạng thái hiện tại của Robot lên đám mây (chuỗi string).
- **V5 (Input)**: Nhận lệnh phát nhạc từ Dashboard Blynk (1: Mario, 2: Despacito, 3: Jingle Bells, 0: Tắt nhạc và dừng nhảy).

---

## II. CÁC NHÓM TRẠNG THÁI MÔI TRƯỜNG & KỊCH BẢN PHÁT NHẠC

### 1. Nhóm Trạng Trạng Thái Tiêu Chuẩn (Nơi làm việc lý tưởng)

#### Trường hợp 1.1: Hoàn hảo hoàn toàn (Ban ngày)
- **Điều kiện:** Nhiệt độ ($22^\circ\text{C} \le T \le 26^\circ\text{C}$) **AND** Độ ẩm ($40\% \le H \le 65\%$) **AND** Ánh sáng ($> 300\text{ lux}$).
- **Robot phản ứng:**
  - **Màn hình:** Mắt tròn xoe vui vẻ hoặc chớp mắt nhẹ.
  - **Đèn LED:** Màu Xanh lá sáng hiệu ứng "nhịp thở" (breathing).
  - **Khớp đầu:** Thỉnh thoảng xoay trái/phải $15^\circ$ ngẫu nhiên sau mỗi 3 phút.
  - **Còi:** Tắt.

#### Trường hợp 1.2: Thư giãn / Chế độ ban đêm (Ban đêm)
- **Điều kiện:** Nhiệt độ và Độ ẩm lý tưởng **AND** Ánh sáng tối ($< 50\text{ lux}$).
- **Robot phản ứng:**
  - **Màn hình:** Chuyển sang mắt nhắm `(- _ -)`.
  - **Đèn LED:** Chuyển sang màu Tím mờ (giảm độ sáng xuống 10%).
  - **Khớp đầu:** Quay về chính giữa ($90^\circ$) và đứng yên.
  - **Còi:** Tắt.

---

### 2. Nhóm Cảnh Báo Môi Trường (Tích hợp âm nhạc cảnh báo)

#### Trường hợp 2.1: Quá nóng hoặc Ngột ngạt (Hiệu ứng nhà kính)
- **Điều kiện:** Nhiệt độ thực tế $T > 30^\circ\text{C}$ **OR** Nhiệt độ cảm nhận $T_{\text{feel}} > 33^\circ\text{C}$.
- **Robot phản ứng:**
  - **Màn hình:** Mắt mệt mỏi, rủ xuống `(~ _ ~)`.
  - **Đèn LED:** Chuyển sang màu Cam.
  - **Khớp đầu:** Xoay chậm sang trái rồi sang phải mệt mỏi.
  - **Còi:** Ngay khi chuyển sang trạng thái này, robot sẽ tự động phát nhạc bài **Despacito** một lần (không chặn). Sau khi nhạc kết thúc, nếu tiếp tục ở trạng thái nóng, còi kêu 1 tiếng "bíp" ngắn nhắc nhở sau mỗi 5 phút.

#### Trường hợp 2.2: Lạnh và Khô (Phòng điều hòa quá đà)
- **Điều kiện:** Nhiệt độ $T < 18^\circ\text{C}$ **AND** Độ ẩm $H < 35\%$.
- **Robot phản ứng:**
  - **Màn hình:** Mắt run rẩy (`HFlicker` kích hoạt).
  - **Đèn LED:** Màu Xanh lam nhạt (70% độ sáng).
  - **Khớp đầu:** Rung lắc nhẹ nhanh liên tục giữa góc 85° và 95° giả vờ run rẩy.
  - **Còi:** Ngay khi chuyển sang trạng thái này, robot tự động phát bản nhạc **Jingle Bells** một lần (không chặn) để cảnh báo lạnh.

#### Trường hợp 2.3: Thiếu sáng khi đang làm việc (Cảnh báo mỏi mắt)
- **Điều kiện:** Ánh sáng yếu ($50\text{ lux} \le \text{Ánh sáng} < 150\text{ lux}$) **AND** duy trì liên tục quá 10 phút.
- **Robot phản ứng:**
  - **Màn hình:** Mắt nheo lại giận dữ (`ANGRY`) ngước nhìn lên trên.
  - **Đèn LED:** Chớp tắt màu Vàng chậm (mỗi 500ms).
  - **Khớp đầu:** Ngước lên trên ($90^\circ$).
  - **Còi:** Kêu tiếng bíp đôi dồn dập sau mỗi 1 phút để nhắc nhở bật đèn.

---

### 3. Nhóm Cảnh Báo Nguy Hiểm (Mức độ ưu tiên cao nhất - Ngắt nhạc tức thì)

#### Trường hợp 3.1: Nguy cơ hỏa hoạn
- **Điều kiện:** Nhiệt độ thực tế $T > 42^\circ\text{C}$.
- **Robot phản ứng:**
  - **Màn hình:** Mắt chữ X hoảng loạn `(X _ X)` vẽ trực tiếp.
  - **Đèn LED:** Đỏ chớp nháy liên tục với tần số cao (Strobe 100ms).
  - **Khớp đầu:** Xoay liên tục dồn dập giữa góc 60° và 120° tốc độ cao.
  - **Còi:** Hú báo động dồn dập (thay đổi tần số 2500Hz - 1800Hz liên tục) và **ngắt hoàn toàn mọi bài hát đang phát**.

#### Trường hợp 3.2: Độ ẩm quá cao (Nguy cơ hỏng thiết bị)
- **Điều kiện:** Độ ẩm $H > 85\%$.
- **Robot phản ứng:**
  - **Màn hình:** Mắt rủ buồn bã kèm đổ mồ hôi.
  - **Đèn LED:** Màu Đỏ sáng cố định (100% độ sáng).
  - **Khớp đầu:** Quay hẳn sang góc 60° (tránh nguồn phun ẩm) và đứng yên.
  - **Còi:** Phát còi cảnh báo kéo dài liên tục ở 1000Hz và **ngắt hoàn toàn mọi bài hát đang phát**.

---

### 4. Tương tác đặc biệt của người dùng
- **Chạm giữ 3 giây (Long Press)**: Robot chuyển sang chế độ nhảy múa (`DANCE_MODE`), quay đầu linh hoạt, đèn LED hiệu ứng cầu vồng và phát nhạc bài **Super Mario** bằng buzzer. Khi hết nhạc, robot tự trả về trạng thái bình thường.
- **Điều khiển qua Blynk (Pin V5)**: Phát bài hát tương ứng (1: Mario, 2: Despacito, 3: Jingle Bells) kèm theo nhảy múa, gửi `0` để tắt ngay lập tức.

---

## III. THAM KHẢO TƯ DUY VIẾT CODE CẬP NHẬT (PSEUDO-CODE)

```cpp
// ==========================================
// CẤU TRÚC DỮ LIỆU BÀI HÁT (songs.h)
// ==========================================
STRUCT Note {
    uint16_t pitch
    uint8_t duration // 4: nốt đen, 8: nốt móc đơn...
}

CONSTANT Note mario_melody[] = { ... }
CONSTANT Note despacito_melody[] = { ... }
CONSTANT Note jingle_bells_melody[] = { ... }

// ==========================================
// TRÌNH PHÁT NHẠC KHÔNG CHẶN TRONG ACTUATORS (actuators.h)
// ==========================================
CLASS Actuators {
    PRIVATE:
        int currentSong = 0       // 0: Idle, 1: Mario, 2: Despacito, 3: Jingle Bells
        int currentNoteIndex = 0
        unsigned long nextNoteTime = 0
        bool isSilentGap = false

    PUBLIC:
        FUNCTION playSong(songId):
            currentSong = songId
            currentNoteIndex = 0
            nextNoteTime = Thời_gian_hiện_tại()
            isSilentGap = false
            stopTone()

        FUNCTION stopSong():
            currentSong = 0
            stopTone()

        FUNCTION isSongPlaying():
            RETURN currentSong != 0

        FUNCTION updateBuzzer():
            // ƯU TIÊN SỐ 1: Trạng thái nguy hiểm (Hỏa hoạn, Độ ẩm cao)
            IF (currentState == DANGER_FIRE) THEN
                stopSong() // Hủy nhạc ngay lập tức
                Chạy_còi_hú_báo_cháy()
                RETURN
            END IF
            
            IF (currentState == DANGER_HUMID) THEN
                stopSong() // Hủy nhạc ngay lập tức
                Chạy_còi_cảnh_báo_ẩm()
                RETURN
            END IF

            // ƯU TIÊN SỐ 2: Phát nhạc không chặn
            IF (currentSong != 0) THEN
                IF (Thời_gian_hiện_tại() >= nextNoteTime) THEN
                    Note n = Lấy_nốt_nhạc(currentSong, currentNoteIndex)
                    unsigned long noteDuration = 240000 / (Bản_nhạc_Tempo * n.duration)
                    
                    IF (NOT isSilentGap) THEN
                        IF (n.pitch > 0) THEN playTone(n.pitch)
                        ELSE stopTone()
                        nextNoteTime = Thời_gian_hiện_tại() + (noteDuration * 0.9)
                        isSilentGap = true
                    ELSE
                        stopTone()
                        nextNoteTime = Thời_gian_hiện_tại() + (noteDuration * 0.1)
                        isSilentGap = false
                        currentNoteIndex++
                    END IF
                END IF
                RETURN
            END IF

            // Còi bíp bình thường
            Chạy_tiếng_bíp_chuỗi_ngắn()
}

// ==========================================
// BỘ ĐIỀU KHIỂN CHÍNH (controller.h)
// ==========================================
CLASS Controller {
    FUNCTION update():
        IF (isDancing) THEN
            // Tự động kết thúc nhảy múa khi bài hát kết thúc
            IF (Thời_gian_hiện_tại() >= danceEndTime OR NOT actuators.isSongPlaying()) THEN
                isDancing = false
                actuators.stopSong()
                currentState = preDanceState
                Cập_nhật_trạng_thái_linh_kiện()
            END IF
        END IF
        
    FUNCTION evaluateState():
        // ... Đánh giá trạng thái thời tiết ...
        IF (nextState != currentState) THEN
            currentState = nextState
            Cập_nhật_trạng_thái_linh_kiện()
            
            // Tự động phát nhạc khi đổi trạng thái thời tiết
            IF (currentState == WARNING_HOT) THEN
                actuators.playSong(2) // Phát Despacito
            ELSE IF (currentState == WARNING_COLD) THEN
                actuators.playSong(3) // Phát Jingle Bells
            ELSE
                actuators.stopSong() // Tắt nhạc khi về bình thường
            END IF
        END IF
}

// Callback từ Blynk nhận dữ liệu chân ảo V5
GLOBAL FUNCTION BLYNK_WRITE(V5):
    int songId = param.asInt()
    robotController.playSongBlynk(songId)
```