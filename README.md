# 🤖 IOT-DeskPet (Robot Thú Cưng Để Bàn Thông Minh)

**IOT-DeskPet** là một dự án robot để bàn tương tác thông minh được phát triển trên vi điều khiển **ESP32**. Robot tích hợp màn hình hiển thị biểu cảm, hệ thống đèn LED hiệu ứng, động cơ servo chuyển động đầu linh hoạt, các cảm biến môi trường và kết nối Cloud qua **Blynk IoT** để giám sát thông số từ xa.

---

## 🌟 Các Tính Năng Nổi Bật

1. **Biểu Cảm Khuôn Mặt Sinh Động (OLED SSD1306 + RoboEyes)**: Mắt robot tự chớp, nhìn xung quanh ngẫu nhiên và biến đổi tâm trạng linh hoạt dựa trên điều kiện môi trường hoặc sự tương tác từ người dùng.
2. **Hệ Thống Phản Hồi Trực Quan (NeoPixel LED & Buzzer)**:
   - Vòng 12 LED WS2812B thay đổi màu sắc và hiệu ứng nhịp thở/chớp nháy tương ứng với từng trạng thái của robot.
   - Còi Buzzer phát âm thanh cảnh báo độc đáo và chơi nhạc các bài hát quen thuộc (*Super Mario*, *Despacito*, *Jingle Bells*).
3. **Cử Động Đầu Tự Nhiên (Servo SG90)**: Robot lắc đầu phản hồi khi được tương tác hoặc xoay đầu tự động để biểu thị tâm trạng.
4. **Giám Sát Môi Trường Từ Xa**: Đo đạc nhiệt độ, độ ẩm (DHT11) và cường độ ánh sáng (BH1750), tự động đồng bộ hóa lên Cloud qua Blynk.
5. **Cấu Hình Chạm Đa Năng (TTP223 Touch Sensor)**: Cho phép chuyển chế độ, tắt âm cảnh báo, bật màn hình thông số, nháy mắt hoặc nhảy múa bằng các thao tác chạm/giữ.
6. **Chế Độ Test Tiện Lợi (Test Mode)**: Giả lập các điều kiện môi trường cực đoan (cháy, nóng, lạnh...) để kiểm tra phản ứng của robot mà không cần dùng lửa hay thiết bị nhiệt độ thật.

---

## 🔌 Sơ Đồ Nối Dây (Pinout Connections)

Dưới đây là cấu hình chân mặc định trên board **ESP32 DevKit V1**:

| Linh Kiện | Chân Trên ESP32 | Mô Tả |
| :--- | :--- | :--- |
| **TTP223 Touch Sensor** | `GPIO 15` | Cảm biến chạm tương tác |
| **DHT11 Sensor** | `GPIO 19` | Cảm biến nhiệt độ & độ ẩm |
| **Servo SG90** | `GPIO 14` | Động cơ quay đầu |
| **NeoPixel LED Ring** | `GPIO 4` | Vòng 12 LED WS2812B |
| **Buzzer** | `GPIO 18` | Còi phát âm thanh / nhạc |
| **OLED Screen & BH1750** | `SDA (GPIO 21)`, `SCL (GPIO 22)` | Giao tiếp I2C hiển thị & cảm biến ánh sáng |

---

## 👆 Hướng Dẫn Sử Dụng Phím Chạm (Touch Gestures Guide)

Cảm biến chạm được tích hợp hệ thống nhận diện cử chỉ thông minh:

### 🔄 Chuyển Chế Độ (Nhấn Giữ 2 Giây)
- **Nhấn giữ cảm biến chạm trong 2 giây**:
  - **Bật Chế Độ Test**: Robot phát **2 tiếng bíp nhanh**, còi/nhạc giả lập sẵn sàng.
  - **Tắt Chế Độ Test**: Trở lại chế độ đọc cảm biến thực tế, robot phát **1 tiếng bíp dài**.

---

### 🟢 Khi Ở Chế Độ Thường (Normal Mode)
- **1 Chạm**: 
  - *Nếu còi báo động hoặc nhạc cảnh báo đang kêu*: **Tắt âm thanh (Muted)** của trạng thái hiện tại.
  - *Nếu không có âm thanh cảnh báo*: **Bật/Tắt màn hình hiển thị thông số môi trường** (Nhiệt độ, Độ ẩm, Ánh sáng dưới dạng thanh tiến trình trực quan).
- **2 Chạm**: Ra lệnh robot nháy mắt (Wink) kết hợp lắc đầu nhẹ.
- **3 Chạm**: Kích hoạt **Dance Mode** trong **5 giây** (Robot nhảy múa xoay đầu, Led chạy hiệu ứng cầu vồng và còi phát nhạc nền *Super Mario*).

---

### 🔴 Khi Ở Chế Độ Test (Test Mode)
- **1 Chạm**: Chuyển đổi tuần hoàn qua các trạng thái giả lập môi trường để test hoạt động của robot:
  1. `NORMAL_HAPPY` (Bình thường - Không giả lập)
  2. `DANGER_FIRE` (Cảnh báo cháy: Led chớp đỏ dồn dập, còi hú siren khẩn cấp)
  3. `DANGER_HUMID` (Cảnh báo ẩm cao: Led đỏ đặc, còi kêu liên tục 1000Hz)
  4. `WARNING_HOT` (Cảnh báo nóng: Led cam, phát nhạc *Despacito* và bíp nhắc)
  5. `WARNING_COLD` (Cảnh báo lạnh: Led cyan, đầu shivering, phát nhạc *Jingle Bells*)
  6. `SLEEP_MODE` (Chế độ ngủ: Mắt nhắm, Led tím mờ)
  7. `WARNING_DARK` (Cảnh báo tối lâu: Led nháy vàng, còi kêu bíp đôi định kỳ)
  - Sau trạng thái 7 sẽ quay lại trạng thái 1.

---

## 📊 Sơ Đồ Thuật Toán & Luồng Xử Lý (System Flowcharts)

File sơ đồ thiết kế chi tiết (Draw.io): [`flowchart.drawio`](flowchart.drawio)

### 1. Sơ đồ Tổng quan Bộ não Robot — Hàm `update()`
```mermaid
flowchart TD
    A([Bắt đầu vòng lặp loop]) --> B[1. Đọc dữ liệu cảm biến: sensors.update]
    B --> C{Đang nhảy múa isDancing?}
    
    C -- Có --> D{Hết 10s HOẶC Nhạc đã tắt?}
    D -- Có --> E[Dừng nhảy múa: stopDanceMode & Khôi phục tâm trạng cũ] --> M1(( ))
    D -- Không --> M1
    
    C -- Không --> F[2. Đánh giá Môi trường & Đổi tâm trạng: evaluateEnvironmentAndUpdateState] --> M1
    
    M1 --> G[4. Nhận biết & Xử lý Cảm ứng: updateTouchGesture]
    G --> H[5. Cập nhật Động cơ Servo, Đèn LED & Đôi mắt OLED]
    H --> I[6. Gửi dữ liệu cảm biến & tâm trạng lên App Blynk IoT]
    I --> J([Kết thúc 1 chu kỳ - Lặp lại])
```

---

### 2. Phản ứng Môi trường — Hàm `evaluateEnvironmentAndUpdateState()`
```mermaid
flowchart TD
    Start([Bắt đầu]) --> Read[Lấy thông số: Nhiệt độ, Độ ẩm, Cảm giác nhiệt, Ánh sáng Lux]
    
    Read --> D1{Nhiệt độ > 42°C?}
    D1 -- Có --> S1[targetState = DANGER_FIRE<br/>Cháy khẩn cấp] --> TargetJoin(( ))
    
    D1 -- Không --> D2{Độ ẩm > 85%?}
    D2 -- Có --> S2[targetState = DANGER_HUMID<br/>Ẩm ướt hư mạch] --> TargetJoin
    
    D2 -- Không --> D3{NĐ > 35°C HOẶC Cảm giác > 38°C?}
    D3 -- Có --> S3[targetState = WARNING_HOT<br/>Nóng nực mệt mỏi] --> TargetJoin
    
    D3 -- Không --> D4{NĐ < 18°C VÀ ĐẨ < 36%?}
    D4 -- Có --> S4[targetState = WARNING_COLD<br/>Lạnh run người] --> TargetJoin
    
    D4 -- Không --> D5{Ánh sáng < 50 Lux?}
    D5 -- Có --> S5[targetState = SLEEP_MODE<br/>Nhắm mắt đi ngủ] --> TargetJoin
    
    D5 -- Không --> D6{Ánh sáng < 150 Lux<br/>VÀ Tối liên tục > 10 phút?}
    D6 -- Có --> S6[targetState = WARNING_DARK<br/>Tối lâu hại mắt] --> TargetJoin
    
    D6 -- Không --> SDef[targetState = NORMAL_HAPPY<br/>Môi trường lý tưởng, vui vẻ] --> TargetJoin
    
    TargetJoin --> DChange{targetState != currentState?<br/>Tâm trạng mục tiêu khác hiện tại?}
    DChange -- Có --> Apply[Cập nhật Tâm trạng mới: changeState<br/>• Đổi biểu cảm Mắt OLED, Đèn LED & Cổ Servo<br/>• Bật nhạc dỗ dành Despacito / Jingle Bells] --> EndJoin(( ))
    DChange -- Không --> EndJoin
    
    EndJoin --> Finish([Kết thúc])
```

---

### 3. Nhận diện & Xử lý Cảm ứng — Hàm `updateTouchGesture()`
```mermaid
flowchart TD
    Start([Bắt đầu]) --> D1{Đang nhảy múa?}
    D1 -- Có --> MEnd(( ))
    
    D1 -- Không --> ReadTouch[Đọc trạng thái cảm ứng: isTouched]
    ReadTouch --> D2{Giữ lâu ≥ 2 giây?}
    
    D2 -- Có --> HoldP[Bật / Tắt chế độ Test<br/>tapCount = 0<br/>Phát tiếng bíp xác nhận] --> MEnd
    
    D2 -- Không --> D3{Chạm hợp lệ?<br/>nhả ra, 50ms – 600ms}
    D3 -- Có --> ValidP[tapCount++<br/>Ghi nhận thời gian chạm cuối] --> M1(( ))
    D3 -- Không --> M1
    
    M1 --> D4{tapCount > 0 VÀ đã qua 400ms?}
    D4 -- Không --> MEnd
    
    D4 -- Có --> D5{tapCount == 1?}
    D5 -- Có --> Tap1[Xử lý Chạm 1 lần: handleSingleTap<br/>tapCount = 0] --> MEnd
    
    D5 -- Không --> D6{tapCount == 2?}
    D6 -- Có --> Tap2[Xử lý Chạm 2 lần: handleDoubleTap<br/>tapCount = 0] --> MEnd
    D6 -- Không --> Tap3[Xử lý Chạm 3 lần: handleTripleTap<br/>tapCount = 0] --> MEnd
    
    MEnd --> Finish([Kết thúc])
```

---

### 4. Xử lý Chạm 1 lần — Hàm `handleSingleTap()`
```mermaid
flowchart TD
    Start([Bắt đầu]) --> D1{Đang ở chế độ Test?}
    
    D1 -- Có --> TestP[Chuyển sang kịch bản giả lập tiếp theo<br/>setMock → trạng thái thời tiết kế tiếp<br/>alarmMuted = false] --> MEnd(( ))
    
    D1 -- Không --> D2{Đang có báo động?<br/>!alarmMuted VÀ đang ở trạng thái NGUY HIỂM hoặc đang hát}
    D2 -- Có --> MuteP[Tắt tiếng báo động<br/>alarmMuted = true] --> MEnd
    
    D2 -- Không --> ToggleP[Bật / Tắt màn hình thông số môi trường<br/>Phát tiếng bíp xác nhận] --> MEnd
    
    MEnd --> Finish([Kết thúc])
```

---

### 5. Xử lý Chạm 2 lần & 3 lần — Hàm `handleDoubleTap()` & `handleTripleTap()`

#### 🔹 `handleDoubleTap()` (Chạm 2 lần - Nháy mắt & Lắc đầu)
```mermaid
flowchart TD
    Start([Bắt đầu]) --> D1{Đang hiện màn hình thông số?}
    D1 -- Có --> Ignore[Bỏ qua] --> MEnd(( ))
    D1 -- Không --> Action[Nháy mắt Wink<br/>Lắc đầu nhẹ nhàng<br/>Phát bíp đôi 2500 Hz] --> MEnd
    MEnd --> Finish([Kết thúc])
```

#### 🔹 `handleTripleTap()` (Chạm 3 lần - Kích hoạt Nhảy múa Dance Mode)
```mermaid
flowchart TD
    Start([Bắt đầu]) --> D1{Đang hiện màn hình thông số?}
    D1 -- Có --> HideP[Ẩn màn hình thông số<br/>Lưu cờ: quay lại sau khi nhảy xong] --> M1(( ))
    D1 -- Không --> ClearP[Xóa cờ: không cần quay lại màn hình thông số] --> M1
    
    M1 --> DanceP[Kích hoạt nhảy múa Dance Mode<br/>Phát bài nhạc Super Mario<br/>Đặt hẹn giờ nhảy = 5 giây] --> Finish([Kết thúc])
```

---


## 🛠️ Hướng Dẫn Cài Đặt & Cấu Hình

### 1. Thư viện yêu cầu (Arduino IDE / PlatformIO)
Hãy chắc chắn rằng bạn đã cài đặt đầy đủ các thư viện sau:
- `Adafruit SSD1306` & `Adafruit GFX Library`
- `Adafruit NeoPixel`
- `ESP32Servo` (by Kevin Harrington)
- `BH1750` (by Christopher Laws)
- `DHT sensor library` (by Adafruit)
- `Blynk` (by Volodymyr Shymanskyy)

### 2. Cấu hình WiFi & Blynk
Mở file [blynk_service.h](file:///d:/Materials/4_Semester/IOT102/Project/Project/blynk_service.h) và cập nhật thông tin cá nhân:
```cpp
// Thông tin Blynk Cloud của bạn
#define BLYNK_TEMPLATE_ID "TEMPLATE_ID_CỦA_BẠN"
#define BLYNK_TEMPLATE_NAME "TÊN_TEMPLATE_CỦA_BẠN"
#define BLYNK_AUTH_TOKEN "AUTH_TOKEN_CỦA_BẠN"

// Cấu hình Wi-Fi nhà bạn
#define WIFI_SSID "Tên_Wifi_Của_Bạn"
#define WIFI_PASS "Mật_Khẩu_Wifi_Của_Bạn"
```

Sau đó biên dịch dự án và nạp code xuống board ESP32 của bạn!

---

## 👥 Thành Viên Dự Án (Project Contributors)

| Họ và Tên | Mã Sinh Viên (MSSV) | Vai Trò |
| :--- | :--- | :--- |
| **Nguyễn Huy Nhật** | HE204465 | Thành viên nhóm |
| **Lưu Chí Kiên** | HE204365 | Thành viên nhóm |
| **Phạm Công Hùng** | HE204376 | Thành viên nhóm |

