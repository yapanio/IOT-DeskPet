#ifndef SENSORS_H
#define SENSORS_H

#include <BH1750.h>
#include <DHT.h>
#include <Wire.h>

#define DHTTYPE DHT11

class Sensors {
 private:
  DHT dht;
  BH1750 lightMeter;

  float temperature = 0.0;
  float humidity    = 0.0;
  float heatIndex   = 0.0;
  float lightLux    = 0.0;

  unsigned long lastDhtRead   = 0;
  unsigned long lastLightRead = 0;

  const unsigned long DHT_READ_INTERVAL   = 2000;
  const unsigned long LIGHT_READ_INTERVAL = 1000;

  bool  mockMode  = false;
  float mockTemp  = 25.0;
  float mockHumid = 55.0;
  float mockLux   = 350.0;

 public:
  Sensors(int dhtPin) : dht(dhtPin, DHTTYPE) {}

  void begin() {
    dht.begin();

    Wire.begin();
    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23)) {
      Serial.println(F("[Cảm biến] Mắt đo ánh sáng BH1750 đã sẵn sàng!"));
    } else {
      Serial.println(F("[Cảm biến] Ối! Không tìm thấy mắt đo ánh sáng BH1750 rồi!"));
    }
  }

  void setMock(bool enable, float temp, float humid, float lux) {
    mockMode  = enable;
    mockTemp  = temp;
    mockHumid = humid;
    mockLux   = lux;
  }

  bool isMockEnabled() const { return mockMode; }

  void update() {
    if (mockMode) {
      temperature = mockTemp;
      humidity    = mockHumid;
      heatIndex   = dht.computeHeatIndex(temperature, humidity, false);
      lightLux    = mockLux;
      return;
    }

    unsigned long now = millis();

    if (now - lastDhtRead >= DHT_READ_INTERVAL || lastDhtRead == 0) {
      float tempRead  = dht.readTemperature();
      float humidRead = dht.readHumidity();

      if (!isnan(tempRead) && !isnan(humidRead)) {
        temperature = tempRead;
        humidity    = humidRead;
        heatIndex   = dht.computeHeatIndex(temperature, humidity, false);
      } else {
        Serial.println(F("[Cảm biến] Cảnh báo: Lỗi không đọc được cảm biến DHT11 rồi!"));
      }
      lastDhtRead = now;
    }

    if (now - lastLightRead >= LIGHT_READ_INTERVAL || lastLightRead == 0) {
      float luxRead = lightMeter.readLightLevel();
      if (luxRead >= 0) {
        lightLux = luxRead;
      } else {
        Serial.println(F("[Cảm biến] Cảnh báo: Lỗi không đọc được mắt ánh sáng BH1750!"));
      }
      lastLightRead = now;
    }
  }

  float getTemperature() const { return temperature; }
  float getHumidity() const { return humidity; }
  float getHeatIndex() const { return heatIndex; }
  float getLightLux() const { return lightLux; }
};

#endif
