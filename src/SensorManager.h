#ifndef SENSORMANAGER_H
#define SENSORMANAGER_H

#include <Arduino.h>
#include "DHT.h"
#include "Config.h"

class SensorManager {
private:
    DHT dht;
    
    // Simulation mode values
    float simulatedTemperature;
    float simulatedHumidity;
    int simulatedSoilPercentage;
    int simulatedWaterPercentage;
    
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
