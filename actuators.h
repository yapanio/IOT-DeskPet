/**
 * @file actuators.h
 * @brief Điều khiển ĐÈN LED NeoPixel và CÒI (Buzzer).
 *
 * File này chứa class Actuators chịu trách nhiệm:
 *  - Vòng đèn LED NeoPixel 12 bóng: Hiển thị màu sắc và hiệu ứng
 *    khác nhau tùy theo trạng thái của robot.
 *  - Còi Buzzer: Phát tiếng bíp cảnh báo và phát nhạc (Mario, Despacito...)
 *    theo kiểu NON-BLOCKING (không làm nghẽn chương trình).
 *
 * THẾ NÀO LÀ "NON-BLOCKING"?
 *  - BLOCKING (chặn): Gọi hàm -> chương trình dừng lại chờ -> xong rồi mới tiếp.
 *    Ví dụ: delay(1000) làm robot không thể làm gì trong 1 giây.
 *  - NON-BLOCKING (không chặn): Hàm hoạt động từng bước nhỏ mỗi lần gọi,
 *    không cần dừng chương trình. Robot vẫn đọc cảm biến, xử lý chạm, v.v.
 *    trong khi nhạc đang phát.
 *
 * PHẦN CỨNG:
 *  - NeoPixel WS2812B: 12 LED, kết nối GPIO 4 (LED_PIN)
 *  - Buzzer: Kết nối GPIO 18 (BUZZER_PIN), điều khiển qua LEDC PWM
 */

#ifndef ACTUATORS_H
#define ACTUATORS_H

#include "robot_state.h"
#include "songs.h"              // Dữ liệu nốt nhạc của các bài hát
#include <Adafruit_NeoPixel.h>  // Thư viện điều khiển đèn LED NeoPixel
#include <esp_arduino_version.h> // Kiểm tra phiên bản ESP32 Arduino SDK

// Số lượng đèn LED trên vòng
#define LED_COUNT 12

/**
 * @class Actuators
 * @brief Quản lý toàn bộ đầu ra: LED và Buzzer.
 */
class Actuators {
 private:
  // ===================================================
  // PHẦN CỨNG
  // ===================================================
  Adafruit_NeoPixel strip;  // Đối tượng vòng LED NeoPixel
  int buzzerPin;            // Chân GPIO của còi Buzzer

  RobotState currentState = NORMAL_HAPPY; // Trạng thái hiện tại

  // ===================================================
  // TRẠNG THÁI LED
  // ===================================================
  unsigned long lastLedUpdate = 0; // Thời điểm cập nhật LED lần cuối

  // ===================================================
  // TRẠNG THÁI TẮT ÂM (Mute)
  // ===================================================
  // Khi muted = true: tất cả âm thanh cảnh báo bị tắt
  // (đèn LED vẫn hoạt động bình thường)
  bool muted = false;

  // ===================================================
  // PHÁT NHẠC NON-BLOCKING
  // ===================================================
  // Phát nhạc kiểu state-machine: mỗi lần gọi update(),
  // hàm xử lý nốt nhạc tiếp theo nếu đã đến lúc.
  int  currentSong       = 0;    // Bài đang phát (0=không phát, 1=Mario, 2=Despacito, 3=Jingle Bells)
  int  currentNoteIndex  = 0;    // Đang ở nốt thứ mấy trong bài?
  unsigned long nextNoteTime = 0; // Thời điểm phát nốt tiếp theo (ms)
  bool isSilentGap       = false; // Đang trong khoảng nghỉ giữa 2 nốt không?

  // ===================================================
  // BÍP NGẮN / BÍP ĐÔI (One-shot Beep)
  // ===================================================
  // Dùng state-machine để phát bíp mà không cần delay()
  // Các bước: 0=rảnh, 1=bíp 1 đang phát, 2=khoảng nghỉ, 3=bíp 2 đang phát
  unsigned long buzzerSeqStart = 0; // Thời điểm bắt đầu chuỗi bíp (ms)
  int  buzzerSeqStep = 0;  // Bước hiện tại trong chuỗi bíp
  int  buzzerFreq    = 0;  // Tần số bíp (Hz)
  int  buzzerDur     = 0;  // Thời gian mỗi bíp (ms)
  int  buzzerGap     = 0;  // Khoảng nghỉ giữa 2 bíp (ms), 0 = chỉ bíp 1 lần

  // ===================================================
  // CÒNG HÚ NGUY HIỂM (Alarm Siren)
  // ===================================================
  // DANGER_FIRE: Còi hú luân phiên 2 tần số (như xe cứu hỏa)
  unsigned long lastAlarmToggle = 0;     // Thời điểm đổi tần số lần cuối
  bool          alarmToggleState = false; // Tần số cao hay thấp?

  // ===================================================
  // ÂM LƯỢNG CÒI
  // ===================================================
  // Duty cycle LEDC (0-255). 5 ≈ 2% duty -> âm lượng nhỏ để không quá ồn
  const int BUZZER_VOLUME = 5;

  // ===================================================
  // CÁC HÀM PHÁT/TẮT ÂM THANH (Private - nội bộ)
  // ===================================================
  // Dùng LEDC (LED Control) của ESP32 để tạo sóng vuông tần số f -> tiếng còi.
  // Có 2 phiên bản tùy vào ESP32 Arduino SDK (phiên bản 3+ thay đổi cú pháp).

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  // --- SDK phiên bản 3 trở lên ---
  void playTone(int frequency) {
    if (frequency <= 0) { stopTone(); return; }
    ledcAttach(buzzerPin, frequency, 8);  // Gắn kênh LEDC
    ledcWrite(buzzerPin, BUZZER_VOLUME);  // Bật âm với duty cycle nhỏ
  }
  void stopTone() {
    ledcWrite(buzzerPin, 0);   // Duty = 0 -> tắt âm
    ledcDetach(buzzerPin);     // Ngắt kết nối LEDC
  }
#else
  // --- SDK phiên bản cũ hơn ---
  void playTone(int frequency) {
    if (frequency <= 0) { stopTone(); return; }
    ledcSetup(1, frequency, 8);    // Cấu hình kênh LEDC số 1
    ledcAttachPin(buzzerPin, 1);   // Gắn chân Buzzer vào kênh 1
    ledcWrite(1, BUZZER_VOLUME);   // Bật âm
  }
  void stopTone() {
    ledcWrite(1, 0); // Tắt âm kênh 1
  }
#endif

 public:
  /**
   * @brief Hàm khởi tạo.
   * @param ledPin    Chân GPIO kết nối dây Data của NeoPixel
   * @param buzzerPin Chân GPIO kết nối Buzzer
   */
  Actuators(int ledPin, int buzzerPin)
      : strip(LED_COUNT, ledPin, NEO_GRB + NEO_KHZ800), buzzerPin(buzzerPin) {}

  /**
   * @brief Khởi động LED và Buzzer. Gọi một lần trong setup().
   */
  void begin() {
    strip.begin(); // Khởi động NeoPixel
    strip.show();  // Tắt tất cả LED (all off)

    pinMode(buzzerPin, OUTPUT);
    stopTone(); // Đảm bảo Buzzer tắt lúc bắt đầu
  }

  /**
   * @brief Bật/tắt chế độ im lặng (Mute) cho âm thanh cảnh báo.
   * Khi muted = true: tắt toàn bộ âm thanh ngay lập tức.
   * @param mute true = im lặng, false = bình thường
   */
  void setMuted(bool mute) {
    muted = mute;
    if (muted) {
      stopTone();
      stopSong();
    }
  }

  /** @return Có đang ở chế độ im lặng không? */
  bool isMuted() const { return muted; }

  /**
   * @brief Thông báo trạng thái mới cho Actuators.
   * Mỗi khi robot đổi trạng thái, hàm này reset toàn bộ âm thanh
   * cũ để chuẩn bị cho âm thanh mới phù hợp với trạng thái mới.
   * @param state Trạng thái mới
   */
  void setState(RobotState state) {
    if (currentState == state) return; // Không làm gì nếu không đổi

    currentState = state;

    // Reset toàn bộ âm thanh ngay khi đổi trạng thái
    stopTone();
    stopSong();
    buzzerSeqStep    = 0;
    lastAlarmToggle  = 0;
    alarmToggleState = false;
    muted            = false; // Tự động bỏ mute khi đổi sang trạng thái mới
  }

  /**
   * @brief Bắt đầu phát một bài nhạc (non-blocking).
   * Nhạc sẽ được phát từng nốt trong hàm update().
   * @param songId 1 = Mario, 2 = Despacito, 3 = Jingle Bells
   */
  void playSong(int songId) {
    if (songId >= 1 && songId <= 3) {
      currentSong      = songId;
      currentNoteIndex = 0;
      nextNoteTime     = millis();
      isSilentGap      = false;
      stopTone(); // Tắt âm đang phát (nếu có) để bắt đầu bài mới
    } else {
      stopSong(); // SongId không hợp lệ -> dừng nhạc
    }
  }

  /** @brief Dừng phát nhạc ngay lập tức. */
  void stopSong() {
    currentSong = 0;
    stopTone();
  }

  /** @return Có bài nhạc nào đang phát không? */
  bool isSongPlaying() const { return currentSong != 0; }

  /**
   * @brief Kích hoạt 1 tiếng bíp ngắn (non-blocking).
   * Tiếng bíp sẽ được phát dần trong hàm update().
   * Không hoạt động khi đang ở trạng thái nguy hiểm (DANGER).
   * @param frequency Tần số (Hz), ví dụ: 1200 = tiếng cao
   * @param duration  Thời gian bíp (ms)
   */
  void triggerSingleBeep(int frequency, int duration) {
    if (currentState == DANGER_FIRE || currentState == DANGER_HUMID) return;
    if (buzzerSeqStep != 0) return; // Đang bận với một chuỗi bíp khác

    buzzerFreq    = frequency;
    buzzerDur     = duration;
    buzzerGap     = 0;          // Gap = 0 -> chỉ bíp 1 lần
    buzzerSeqStart = millis();
    buzzerSeqStep  = 1;         // Bắt đầu chuỗi bíp
  }

  /**
   * @brief Kích hoạt 2 tiếng bíp liên tiếp (non-blocking).
   * Tiếng bíp sẽ được phát dần trong hàm update().
   * @param frequency Tần số (Hz)
   * @param duration  Thời gian mỗi bíp (ms)
   * @param gap       Khoảng nghỉ giữa 2 bíp (ms)
   */
  void triggerDoubleBeep(int frequency, int duration, int gap) {
    if (currentState == DANGER_FIRE || currentState == DANGER_HUMID) return;
    if (buzzerSeqStep != 0) return;

    buzzerFreq    = frequency;
    buzzerDur     = duration;
    buzzerGap     = gap;
    buzzerSeqStart = millis();
    buzzerSeqStep  = 1;
  }

  /**
   * @brief Cập nhật LED và Buzzer. Gọi liên tục trong loop().
   */
  void update() {
    updateLEDs();
    updateBuzzer();
  }

 private:
  // ===================================================
  // CÁC HÀM NỘI BỘ (Private)
  // ===================================================

  /**
   * @brief Tạo màu sắc cầu vồng (dùng cho DANCE_MODE).
   * Hàm Wheel chuyển một giá trị 0-255 thành màu RGB dọc theo cầu vồng.
   * @param pos Vị trí trên vòng màu (0-255)
   * @return Màu RGB dưới dạng uint32_t
   */
  uint32_t rainbowColor(byte pos) {
    pos = 255 - pos;
    if (pos < 85)  return strip.Color(255 - pos * 3, 0, pos * 3);
    if (pos < 170) { pos -= 85; return strip.Color(0, pos * 3, 255 - pos * 3); }
    pos -= 170;
    return strip.Color(pos * 3, 255 - pos * 3, 0);
  }

  /**
   * @brief Cập nhật màu sắc đèn LED theo trạng thái hiện tại.
   * Giới hạn tốc độ cập nhật ~33fps (mỗi 30ms) để tránh nhấp nháy.
   */
  void updateLEDs() {
    unsigned long now = millis();
    if (now - lastLedUpdate < 30) return; // Chưa đến lúc cập nhật
    lastLedUpdate = now;

    switch (currentState) {

      case DANGER_FIRE: {
        // Chớp đỏ nhanh - mỗi 100ms bật/tắt (báo động cháy!)
        bool isOn = (now / 100) % 2;
        uint32_t color = isOn ? strip.Color(255, 0, 0) : strip.Color(0, 0, 0);
        for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, color);
        strip.show();
        break;
      }

      case DANGER_HUMID: {
        // Đỏ đặc, không nhấp nháy (cảnh báo ẩm nghiêm trọng)
        uint32_t color = strip.Color(255, 0, 0);
        for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, color);
        strip.show();
        break;
      }

      case WARNING_HOT: {
        // Cam đặc (70% sáng) - cảnh báo nóng
        uint32_t color = strip.Color(178, 115, 0); // R=178, G=115, B=0 → Cam
        for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, color);
        strip.show();
        break;
      }

      case WARNING_COLD: {
        // Cyan nhạt (70% sáng) - cảnh báo lạnh
        uint32_t color = strip.Color(0, 178, 178); // R=0, G=178, B=178 → Cyan
        for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, color);
        strip.show();
        break;
      }

      case WARNING_DARK: {
        // Vàng nhấp nháy chậm - mỗi 500ms bật/tắt (cảnh báo tối)
        bool isOn = (now / 500) % 2;
        uint32_t color = isOn ? strip.Color(255, 255, 0) : strip.Color(0, 0, 0);
        for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, color);
        strip.show();
        break;
      }

      case SLEEP_MODE: {
        // Tím mờ (10% sáng) - chế độ ngủ yên tĩnh
        uint32_t color = strip.Color(25, 0, 25); // R=25, G=0, B=25 → Tím mờ
        for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, color);
        strip.show();
        break;
      }

      case NORMAL_HAPPY: {
        // Hiệu ứng "thở" - màu xanh lá sáng dần rồi tối dần theo chu kỳ 3 giây
        // sin() tạo ra sóng từ -1 đến 1, ta chuyển về 0 đến 1
        float brightness = (sin(now * 2.0 * 3.14159265 / 3000.0) + 1.0) / 2.0;
        int greenValue = (int)(brightness * 255);
        uint32_t color = strip.Color(0, greenValue, 0);
        for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, color);
        strip.show();
        break;
      }

      case DANCE_MODE: {
        // Hiệu ứng cầu vồng xoay - mỗi LED có màu khác nhau và dịch chuyển theo thời gian
        int shift = (now / 5) % 256; // Tốc độ xoay
        for (int i = 0; i < LED_COUNT; i++) {
          byte colorPos = (i * 256 / LED_COUNT + shift) % 256;
          strip.setPixelColor(i, rainbowColor(colorPos));
        }
        strip.show();
        break;
      }
    }
  }

  /**
   * @brief Xử lý âm thanh Buzzer theo kiểu state-machine (non-blocking).
   *
   * Thứ tự ưu tiên âm thanh:
   *  1. DANGER_FIRE  -> Còi hú luân phiên (cao nhất)
   *  2. DANGER_HUMID -> Còi rú liên tục
   *  3. Bài nhạc đang phát
   *  4. Chuỗi bíp ngắn/đôi
   */
  void updateBuzzer() {
    unsigned long now = millis();

    // === ƯU TIÊN 1: DANGER_FIRE - Còi hú siren ===
    if (currentState == DANGER_FIRE) {
      if (muted) { stopTone(); return; } // Đã tắt âm
      if (isSongPlaying()) stopSong();   // Dừng nhạc nếu có

      // Đổi tần số mỗi 120ms: 2500Hz <-> 1800Hz (âm thanh xe cứu hỏa)
      if (now - lastAlarmToggle >= 120) {
        lastAlarmToggle  = now;
        alarmToggleState = !alarmToggleState;
        playTone(alarmToggleState ? 2500 : 1800);
      }
      return;
    }

    // === ƯU TIÊN 2: DANGER_HUMID - Còi rú liên tục ===
    if (currentState == DANGER_HUMID) {
      if (muted) { stopTone(); return; }
      if (isSongPlaying()) stopSong();
      playTone(1000); // Tiếng rú ổn định 1000Hz
      return;
    }

    // === ƯU TIÊN 3: Phát nhạc (non-blocking) ===
    if (currentSong != 0) {
      if (muted) { stopSong(); return; }

      if (now >= nextNoteTime) {
        // Lấy dữ liệu bài nhạc
        const Note* melody = nullptr;
        int melodyLen = 0;
        int tempo     = 120;

        if      (currentSong == 1) { melody = mario_melody;       melodyLen = mario_length;       tempo = mario_tempo; }
        else if (currentSong == 2) { melody = despacito_melody;   melodyLen = despacito_length;   tempo = despacito_tempo; }
        else if (currentSong == 3) { melody = jingle_bells_melody; melodyLen = jingle_bells_length; tempo = jingle_bells_tempo; }

        if (melody && currentNoteIndex < melodyLen) {
          if (!isSilentGap) {
            // Bước 1: Phát nốt nhạc trong 90% thời gian nốt
            uint16_t pitch = melody[currentNoteIndex].pitch;
            unsigned long noteDuration = (currentSong == 2)
                ? melody[currentNoteIndex].duration                  // Despacito dùng ms trực tiếp
                : 240000UL / (tempo * melody[currentNoteIndex].duration); // Các bài khác tính từ tempo

            if (pitch > 0) playTone(pitch); else stopTone(); // Rest note (pitch=0) -> im lặng
            nextNoteTime = now + (unsigned long)(noteDuration * 0.9);
            isSilentGap  = true;
          } else {
            // Bước 2: Im lặng 10% để tách biệt nốt này với nốt tiếp theo
            stopTone();
            unsigned long noteDuration = (currentSong == 2)
                ? melody[currentNoteIndex].duration
                : 240000UL / (tempo * melody[currentNoteIndex].duration);

            nextNoteTime = now + (unsigned long)(noteDuration * 0.1);
            isSilentGap  = false;
            currentNoteIndex++;           // Chuyển sang nốt tiếp theo
          }
        } else {
          stopSong(); // Đã hết bài
        }
      }
      return;
    }

    // === ƯU TIÊN 4: Chuỗi bíp ngắn/đôi ===
    // State machine: Bước 0=rảnh, 1=bíp1 đang phát, 2=đang nghỉ, 3=bíp2 đang phát
    if (buzzerSeqStep == 1) {
      // Bíp lần 1: phát trong buzzerDur ms
      if (now - buzzerSeqStart < (unsigned long)buzzerDur) {
        playTone(buzzerFreq);
      } else {
        stopTone();
        buzzerSeqStart = now;
        buzzerSeqStep  = (buzzerGap > 0) ? 2 : 0; // Có gap? -> Bước 2, không? -> Xong
      }
    } else if (buzzerSeqStep == 2) {
      // Nghỉ giữa 2 tiếng bíp
      stopTone();
      if (now - buzzerSeqStart >= (unsigned long)buzzerGap) {
        buzzerSeqStep  = 3;
        buzzerSeqStart = now;
      }
    } else if (buzzerSeqStep == 3) {
      // Bíp lần 2
      if (now - buzzerSeqStart < (unsigned long)buzzerDur) {
        playTone(buzzerFreq);
      } else {
        stopTone();
        buzzerSeqStep = 0; // Xong chuỗi bíp
      }
    }
  }
};

#endif  // ACTUATORS_H
