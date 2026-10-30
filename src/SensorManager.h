#ifndef SENSORMANAGER_H
#define SENSORMANAGER_H

#include <Arduino.h>
#include <Adafruit_SHT4x.h>
#include <Wire.h>
#include "Config.h"
#if ENABLE_LUX_SENSOR
#include <Adafruit_LTR329_LTR303.h>
#endif

class SensorManager {
private:
    // --- Active sensor: SHT40 ---
    Adafruit_SHT4x sht4;
    bool sht4Found = false;
    float _lastTemp = -999.0f;
    float _lastHumidity = -999.0f;

#if ENABLE_LUX_SENSOR
    Adafruit_LTR303 light;
    bool lightFound = false;
#endif
    float simulatedLux = 500;
    // Simulation mode values
    float simulatedTemperature;
    float simulatedHumidity;
    int simulatedSoilPercentage;
    int simulatedWaterPercentage;
    
    // Grove Water Level Sensor I2C helper methods
    bool getHigh12SectionValue(unsigned char* high_data);
    bool getLow8SectionValue(unsigned char* low_data);
    
public:
    SensorManager();
    void begin();
    void measureSHT40();   // call once per cycle, then use readTemperature/readHumidity
    
    float readLux();
    void setSimulatedLux(float lux);
    float readTemperature();
    float readHumidity();
    int readSoilMoisture();
    int readWaterLevel();
    
    int getSoilPercentage();
    int getWaterPercentage();
    
    // Simulation mode setters
    void setSimulatedTemperature(float temp);
    void setSimulatedHumidity(float hum);
    void setSimulatedSoilPercentage(int soil);
    void setSimulatedWaterPercentage(int water);
};

#endif // SENSORMANAGER_H
