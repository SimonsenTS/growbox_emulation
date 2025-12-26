#ifndef SENSORMANAGER_H
#define SENSORMANAGER_H

#include <Arduino.h>
#include <Adafruit_SHT4x.h>
#include <Wire.h>
#include "Config.h"

class SensorManager {
private:
    Adafruit_SHT4x sht4;
    
    // Simulation mode values
    float simulatedTemperature;
    float simulatedHumidity;
    int simulatedSoilPercentage;
    int simulatedWaterPercentage;
    
    // Grove Water Level Sensor I2C helper methods
    void getHigh12SectionValue(unsigned char* high_data);
    void getLow8SectionValue(unsigned char* low_data);
    
public:
    SensorManager();
    void begin();
    
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
