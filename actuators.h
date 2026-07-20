/**
 * @file actuators.h
 * @brief Cơ thể của Robot - Đèn LED lấp lánh và chiếc còi biết hát!
 *
 * File này giúp điều khiển hai bộ phận biểu diễn của robot:
 *  - Vòng 12 đèn LED NeoPixel: Đổi màu sắc lấp lánh và hiệu ứng nhịp thở siêu đẹp.
 *  - Còi Buzzer phát nhạc: Hát các bài nhạc quen thuộc (Mario, Despacito, Jingle Bells)
 *    và còi báo động mà không làm robot bị đơ hay dừng hoạt động (Non-blocking).
 *
 * PHẦN CỨNG:
 *  - NeoPixel WS2812B: Vòng 12 đèn, cắm ở GPIO 4 (LED_PIN).
 *  - Còi Buzzer: Cắm ở GPIO 18 (BUZZER_PIN), dùng bộ tạo nhạc LEDC PWM của ESP32.
 */

#ifndef ACTUATORS_H
#define ACTUATORS_H

#include "robot_state.h"
#include "songs.h"              // Sách nhạc lưu các nốt nhạc để hát
#include <Adafruit_NeoPixel.h>  // Thư viện giúp tô màu dải đèn LED
#include <esp_arduino_version.h> // Thư viện kiểm tra đời của bộ mạch ESP32

// Vòng của chúng ta có đúng 12 bóng đèn nhỏ
#define LED_COUNT 12

/**
 * @class Actuators
 * @brief Điều khiển đèn LED và còi Buzzer phát âm thanh.
 */
class Actuators {
 private:
  // ===================================================
  // THIẾT BỊ PHẦN CỨNG
  // ===================================================
  Adafruit_NeoPixel strip;  // Dây đèn LED NeoPixel
  int buzzerPin;            // Chân GPIO cắm còi Buzzer

  RobotState currentState = NORMAL_HAPPY; // Cảm xúc hiện tại của robot

  // ===================================================
  // TRẠNG THÁI ĐÈN LED
  // ===================================================
  unsigned long lastLedUpdate = 0; // Thời điểm cuối cùng đèn đổi màu (ms)

  // ===================================================
  // TRẠNG THÁI IM LẶNG (Mute)
  // ===================================================
  // Khi bằng true, robot sẽ tắt nhạc và còi cảnh báo để bé tập trung học bài.
  bool muted = false;

  // ===================================================
  // HÁT NHẠC NON-BLOCKING (Không làm đơ robot)
  // ===================================================
  int  currentSong       = 0;     // Bài hát đang mở (0=tắt, 1=Mario, 2=Despacito, 3=Jingle Bells)
  int  currentNoteIndex  = 0;     // Đang hát tới nốt thứ mấy trong bài?
  unsigned long nextNoteTime = 0;  // Mấy giờ thì hát nốt tiếp theo (ms)?
  bool isSilentGap       = false;  // Có đang nghỉ một tẹo giữa 2 nốt nhạc không?

  // ===================================================
  // KÊU BÍP BÍP ĐƠN / KÊU BÍP BÍP ĐÔI
  // ===================================================
  unsigned long buzzerSeqStart = 0; // Thời điểm bắt đầu kêu bíp (ms)
  int  buzzerSeqStep = 0;  // Bước kêu bíp (0=rảnh, 1=kêu bíp 1, 2=nghỉ, 3=kêu bíp 2)
  int  buzzerFreq    = 0;  // Tần số nốt bíp (Hz)
  int  buzzerDur     = 0;  // Tiếng bíp kéo dài trong bao lâu (ms)
  int  buzzerGap     = 0;  // Thời gian nghỉ giữa 2 tiếng bíp (ms)

  // ===================================================
  // HÚ CÒI BÁO ĐỘNG (DANGER)
  // ===================================================
  unsigned long lastAlarmToggle = 0;     // Lần cuối cùng đổi tiếng còi hú là lúc nào?
  bool          alarmToggleState = false; // Đang hú tiếng còi cao hay còi thấp?

  // ===================================================
  // ĐỘ TO CỦA CÒI (Âm lượng)
  // ===================================================
  // Số 2 ≈ còi kêu rất nhỏ, êm tai không làm bé giật mình. Số càng to kêu càng điếc tai!
  const int BUZZER_VOLUME = 2;

  // ===================================================
  // CÁC HÀM TẠO SÓNG ÂM THANH CHO CÒI (Nội bộ - Private)
  // ===================================================

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  // --- Dành cho mạch ESP32 đời mới ---
  void playTone(int frequency) {
    if (frequency <= 0) { stopTone(); return; }
    ledcAttach(buzzerPin, frequency, 8);  // Cắm loa ảo vào chân còi
    ledcWrite(buzzerPin, BUZZER_VOLUME);  // Bật nhạc kêu nhè nhẹ
  }
  void stopTone() {
    ledcWrite(buzzerPin, 0);   // Tắt nhạc (âm lượng bằng 0)
    ledcDetach(buzzerPin);     // Rút loa ảo ra
  }
#else
  // --- Dành cho mạch ESP32 đời cũ ---
  void playTone(int frequency) {
    if (frequency <= 0) { stopTone(); return; }
    // Thay vì dùng kênh 1 dễ bị xung đột với động cơ cổ Servo làm cổ bị giật giật,
    // ta chuyển sang dùng kênh 4 để cổ robot quay cực kỳ mượt mà nhé!
    ledcSetup(4, frequency, 8);    // Cài đặt kênh phát nhạc số 4
    ledcAttachPin(buzzerPin, 4);   // Cắm còi vào kênh 4
    ledcWrite(4, BUZZER_VOLUME);   // Cho còi kêu nhè nhẹ
  }
  void stopTone() {
    ledcWrite(4, 0); // Tắt âm trên kênh 4
  }
#endif

 public:
  /**
   * @brief Hàm lắp ráp đèn và còi vào cổng thích hợp.
   */
  Actuators(int ledPin, int buzzerPin)
      : strip(LED_COUNT, ledPin, NEO_GRB + NEO_KHZ800), buzzerPin(buzzerPin) {}

  /**
   * @brief Khởi động đèn LED và còi. Gọi một lần lúc bắt đầu cắm điện.
   */
  void begin() {
    strip.begin(); // Đánh thức dải đèn LED
    strip.show();  // Tắt hết đèn cho tối để bắt đầu chơi

    pinMode(buzzerPin, OUTPUT); // Khai báo cổng Buzzer là đầu ra âm thanh
    stopTone(); // Đảm bảo còi không hú bất ngờ lúc mới cắm điện
  }

  /**
   * @brief Bật hoặc tắt âm thanh (Mute).
   */
  void setMuted(bool mute) {
    muted = mute;
    if (muted) {
      stopTone(); // Tắt còi bíp
      stopSong(); // Tắt luôn nhạc
    }
  }

  /** @brief Hỏi xem robot có đang im lặng hay không. */
  bool isMuted() const { return muted; }

  /**
   * @brief Đổi âm thanh và màu đèn theo tâm trạng của robot.
   */
  void setState(RobotState state) {
    if (currentState == state) return; // Không đổi tâm trạng thì thôi giữ nguyên

    currentState = state;

    // Reset lại loa để chuẩn bị phát còi báo động mới hoặc im lặng
    stopTone();
    stopSong();
    buzzerSeqStep    = 0;
    lastAlarmToggle  = 0;
    alarmToggleState = false;
    muted            = false; // Tự động bật lại tiếng khi chuyển sang cảm xúc mới
  }

  /**
   * @brief Mở một bài nhạc.
   * @param songId 1=Mario, 2=Despacito, 3=Jingle Bells
   */
  void playSong(int songId) {
    if (songId >= 1 && songId <= 3) {
      currentSong      = songId;
      currentNoteIndex = 0;
      nextNoteTime     = millis();
      isSilentGap      = false;
      stopTone(); // Tắt âm cũ để hát bài mới
    } else {
      stopSong(); // Bài hát không đúng -> Tắt nhạc luôn
    }
  }

  /** @brief Tắt nhạc ngay lập tức. */
  void stopSong() {
    currentSong = 0;
    stopTone();
  }

  /** @brief Hỏi xem có bài nhạc nào đang phát không. */
  bool isSongPlaying() const { return currentSong != 0; }

  /**
   * @brief Kêu bíp một lần ngắn (như khi con chạm nhẹ vào robot).
   */
  void triggerSingleBeep(int frequency, int duration) {
    if (currentState == DANGER_FIRE || currentState == DANGER_HUMID) return; // Đang báo cháy/ẩm thì không bíp đơn
    if (buzzerSeqStep != 0) return; // Đang bận bíp rồi thì thôi

    buzzerFreq    = frequency;
    buzzerDur     = duration;
    buzzerGap     = 0;          // Không có khoảng cách -> Kêu đúng 1 lần
    buzzerSeqStart = millis();
    buzzerSeqStep  = 1;         // Bắt đầu bước 1
  }

  /**
   * @brief Kêu bíp bíp hai lần (như lời chào vui tươi của robot).
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
   * @brief Cập nhật màu sắc đèn và còi. Hàm này chạy liên tục trong loop().
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
   * @brief Tạo màu sắc cầu vồng rực rỡ cho đèn LED khi nhảy múa.
   */
  uint32_t rainbowColor(byte pos) {
    pos = 255 - pos;
    if (pos < 85)  return strip.Color(255 - pos * 3, 0, pos * 3);
    if (pos < 170) { pos -= 85; return strip.Color(0, pos * 3, 255 - pos * 3); }
    pos -= 170;
    return strip.Color(pos * 3, 255 - pos * 3, 0);
  }

  /**
   * @brief Cập nhật màu sắc của dải đèn LED theo tâm trạng của robot.
   */
  void updateLEDs() {
    unsigned long now = millis();
    if (now - lastLedUpdate < 30) return; // Nháy đèn vừa thôi, chờ 30ms mới nháy tiếp
    lastLedUpdate = now;

    switch (currentState) {

      case DANGER_FIRE: {
        // CỨU HỎA! Chớp đèn đỏ cực nhanh (cứ 100ms lại tắt bật đỏ đen đỏ đen)
        bool isOn = (now / 100) % 2;
        uint32_t color = isOn ? strip.Color(255, 0, 0) : strip.Color(0, 0, 0);
        for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, color);
        strip.show();
        break;
      }

      case DANGER_HUMID: {
        // ƯỚT SŨNG! Đèn đỏ đặc sáng 100% không tắt để báo động
        uint32_t color = strip.Color(255, 0, 0);
        for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, color);
        strip.show();
        break;
      }

      case WARNING_HOT: {
        // NÓNG QUÁ! Đèn màu cam ấm áp sáng 70%
        uint32_t color = strip.Color(178, 115, 0);
        for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, color);
        strip.show();
        break;
      }

      case WARNING_COLD: {
        // LẠNH QUÁ! Đèn màu xanh băng tuyết (Cyan) sáng 70%
        uint32_t color = strip.Color(0, 178, 178);
        for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, color);
        strip.show();
        break;
      }

      case WARNING_DARK: {
        // PHÒNG TỐI QUÁ! Đèn màu vàng nháy chậm chậm cứ 500ms một lần
        bool isOn = (now / 500) % 2;
        uint32_t color = isOn ? strip.Color(255, 255, 0) : strip.Color(0, 0, 0);
        for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, color);
        strip.show();
        break;
      }

      case SLEEP_MODE: {
        // ĐI NGỦ: Đèn màu tím mờ ấm cúng sáng rất dịu (chỉ 10% sáng)
        uint32_t color = strip.Color(25, 0, 25);
        for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, color);
        strip.show();
        break;
      }

      case NORMAL_HAPPY: {
        // VUI VẺ: Đèn màu xanh lá cây "thở" nhịp nhàng (sáng dần lên rồi mờ dần đi trong chu kỳ 3 giây)
        float brightness = (sin(now * 2.0 * 3.14159265 / 3000.0) + 1.0) / 2.0;
        int greenValue = (int)(brightness * 255);
        uint32_t color = strip.Color(0, greenValue, 0);
        for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, color);
        strip.show();
        break;
      }

      case DANCE_MODE: {
        // DANCE! Đèn chạy cầu vồng lấp lánh xoay tròn cực kỳ đẹp mắt
        int shift = (now / 5) % 256;
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
   * @brief Xử lý phát tiếng bíp hoặc bài hát trên còi (Buzzer) từng bước một.
   */
  void updateBuzzer() {
    unsigned long now = millis();

    // === ƯU TIÊN CAO NHẤT: BÁO ĐỘNG CHÁY DANGER_FIRE ===
    if (currentState == DANGER_FIRE) {
      if (muted) { stopTone(); return; } // Nếu bé ấn tắt tiếng -> Tắt luôn
      if (isSongPlaying()) stopSong();   // Tắt nhạc đang hát để nhường chỗ báo cháy

      // Hú còi liên tục kiểu xe cứu hỏa: cứ 120ms đổi giữa tiếng cao (2500Hz) và tiếng thấp (1800Hz)
      if (now - lastAlarmToggle >= 120) {
        lastAlarmToggle  = now;
        alarmToggleState = !alarmToggleState;
        playTone(alarmToggleState ? 2500 : 1800);
      }
      return;
    }

    // === ƯU TIÊN 2: BÁO ĐỘNG ẨM DANGER_HUMID ===
    if (currentState == DANGER_HUMID) {
      if (muted) { stopTone(); return; }
      if (isSongPlaying()) stopSong();
      playTone(1000); // Tiếng rú liên tục đều đặn ở tần số 1000Hz
      return;
    }

    // === ƯU TIÊN 3: PHÁT NHẠC (Bài hát Mario, Despacito...) ===
    if (currentSong != 0) {
      if (muted) { stopSong(); return; }

      if (now >= nextNoteTime) {
        const Note* melody = nullptr;
        int melodyLen = 0;
        int tempo     = 120;

        if      (currentSong == 1) { melody = mario_melody;       melodyLen = mario_length;       tempo = mario_tempo; }
        else if (currentSong == 2) { melody = despacito_melody;   melodyLen = despacito_length;   tempo = despacito_tempo; }
        else if (currentSong == 3) { melody = jingle_bells_melody; melodyLen = jingle_bells_length; tempo = jingle_bells_tempo; }

        if (melody && currentNoteIndex < melodyLen) {
          if (!isSilentGap) {
            // Bước 3.1: Phát nốt nhạc hiện tại (giữ trong 90% thời gian của nốt)
            uint16_t pitch = melody[currentNoteIndex].pitch;
            unsigned long noteDuration = (currentSong == 2)
                ? melody[currentNoteIndex].duration                  // Bài Despacito đã có sẵn mili-giây
                : 240000UL / (tempo * melody[currentNoteIndex].duration); // Các bài khác tính từ nhịp độ (tempo)

            if (pitch > 0) playTone(pitch); else stopTone(); // Nốt lặng thì tắt còi im lặng
            nextNoteTime = now + (unsigned long)(noteDuration * 0.9);
            isSilentGap  = true; // Lần sau gọi update() sẽ tắt còi nghỉ lấy hơi
          } else {
            // Bước 3.2: Nghỉ lấy hơi một tẹo (10% thời gian nốt) để các nốt không bị dính vào nhau
            stopTone();
            unsigned long noteDuration = (currentSong == 2)
                ? melody[currentNoteIndex].duration
                : 240000UL / (tempo * melody[currentNoteIndex].duration);

            nextNoteTime = now + (unsigned long)(noteDuration * 0.1);
            isSilentGap  = false;
            currentNoteIndex++; // Chuẩn bị cho nốt tiếp theo
          }
        } else {
          stopSong(); // Hát hết bài thì tự tắt nhạc
        }
      }
      return;
    }

    // === ƯU TIÊN 4: TIẾNG BÍP CHÀO HỎI bình thường ===
    if (buzzerSeqStep == 1) {
      // Tiếng bíp lần 1 kêu trong buzzerDur ms
      if (now - buzzerSeqStart < (unsigned long)buzzerDur) {
        playTone(buzzerFreq);
      } else {
        stopTone();
        buzzerSeqStart = now;
        buzzerSeqStep  = (buzzerGap > 0) ? 2 : 0; // Nếu có bíp lần 2 (gap > 0) -> chuyển sang bước 2 nghỉ, ngược lại -> tắt còi
      }
    } else if (buzzerSeqStep == 2) {
      // Nghỉ giữa 2 tiếng bíp
      stopTone();
      if (now - buzzerSeqStart >= (unsigned long)buzzerGap) {
        buzzerSeqStep  = 3;
        buzzerSeqStart = now;
      }
    } else if (buzzerSeqStep == 3) {
      // Tiếng bíp lần 2 kêu trong buzzerDur ms
      if (now - buzzerSeqStart < (unsigned long)buzzerDur) {
        playTone(buzzerFreq);
      } else {
        stopTone();
        buzzerSeqStep = 0; // Kêu xong hoàn toàn chuỗi bíp chào hỏi
      }
    }
  }
};

#endif  // ACTUATORS_H
