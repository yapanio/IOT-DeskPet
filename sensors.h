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
  float humidity = 0.0;
  float heatIndex = 0.0;
  float lightLux = 0.0;

  unsigned long lastDhtRead = 0;
  unsigned long lastLightRead = 0;

  const unsigned long dhtInterval =
      2000; // Read temperature/humidity every 2 seconds
  const unsigned long lightInterval = 1000; // Read light sensor every 1 second

  // Mock variables for simulation mode
  bool mockMode = false;
  float mockTemp = 25.0;
  float mockHumid = 55.0;
  float mockLux = 350.0;

public:
  Sensors(int dhtPin) : dht(dhtPin, DHTTYPE) {}

  void begin() {
    dht.begin();
    // Start I2C if not already started
    Wire.begin();
    // Initialize BH1750 light sensor in continuous high resolution mode
    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23)) {
      Serial.println(F("BH1750 initialized successfully"));
    } else {
      Serial.println(F("Error initializing BH1750"));
    }
  }

  void setMock(bool enable, float temp, float humid, float lux) {
    mockMode = enable;
    mockTemp = temp;
    mockHumid = humid;
    mockLux = lux;
  }

  bool isMockEnabled() const { return mockMode; }

  void update() {
    if (mockMode) {
      temperature = mockTemp;
      humidity = mockHumid;
      heatIndex = dht.computeHeatIndex(temperature, humidity, false);
      lightLux = mockLux;
      return;
    }

    unsigned long currentMillis = millis();

    // Non-blocking DHT11 reading
    if (currentMillis - lastDhtRead >= dhtInterval || lastDhtRead == 0) {
      float tempRead = dht.readTemperature();
      float humidRead = dht.readHumidity();

      if (!isnan(tempRead) && !isnan(humidRead)) {
        temperature = tempRead;
        humidity = humidRead;
        heatIndex = dht.computeHeatIndex(temperature, humidity, false);
      } else {
        Serial.println(F("Failed to read from DHT sensor!"));
      }
      lastDhtRead = currentMillis;
    }

    // Non-blocking BH1750 reading
    if (currentMillis - lastLightRead >= lightInterval || lastLightRead == 0) {
      float luxRead = lightMeter.readLightLevel();
      if (luxRead >= 0) {
        lightLux = luxRead;
      } else {
        Serial.println(F("Failed to read from BH1750 sensor!"));
      }
      lastLightRead = currentMillis;
    }
  }

  float getTemperature() const { return temperature; }
  float getHumidity() const { return humidity; }
  float getHeatIndex() const { return heatIndex; }
  float getLightLux() const { return lightLux; }
};

#endif
