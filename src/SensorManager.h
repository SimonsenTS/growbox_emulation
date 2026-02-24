#ifndef SENSORMANAGER_H
#define SENSORMANAGER_H

#include <Arduino.h>
// --- Active sensor: ENS210 (on-PCB, address 0x43) ---
// To switch back to SHT40 breakout: comment the ENS210 lines and uncomment the SHT40 lines
#include <ens210.h>
// #include <Adafruit_SHT4x.h>  // SHT40 breakout (has its own pull-ups - causes conflict with PCB pull-ups)
#include <Wire.h>
#include "Config.h"

class SensorManager {
private:
    // --- Active: ENS210 ---
    ENS210 ens210;
    bool ens210Found = false;
    int _last_t_data = 0, _last_t_status = 0;
    int _last_h_data = 0, _last_h_status = 0;
    // --- Future: SHT40 (uncomment when new PCB without on-board pull-ups is ready) ---
    // Adafruit_SHT4x sht4;
    // bool sht4Found = false;

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
    void measureENS210();   // call once per cycle, then use readTemperature/readHumidity
    
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
