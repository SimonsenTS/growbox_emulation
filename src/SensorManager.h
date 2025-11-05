#ifndef SENSORMANAGER_H
#define SENSORMANAGER_H

#include <Arduino.h>
#include "DHT.h"
#include "Config.h"

class SensorManager {
private:
    DHT dht;
    
public:
    SensorManager();
    void begin();
    
    float readTemperature();
    float readHumidity();
    int readSoilMoisture();
    int readWaterLevel();
    
    int getSoilPercentage();
    int getWaterPercentage();
};

#endif // SENSORMANAGER_H
