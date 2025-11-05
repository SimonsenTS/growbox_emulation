#include "SensorManager.h"

SensorManager::SensorManager() : dht(DHTPIN, DHTTYPE) {
}

void SensorManager::begin() {
    dht.begin();
}

float SensorManager::readTemperature() {
    return dht.readTemperature();
}

float SensorManager::readHumidity() {
    return dht.readHumidity();
}

int SensorManager::readSoilMoisture() {
    return analogRead(SOIL_SENSOR_PIN);
}

int SensorManager::readWaterLevel() {
    return analogRead(WATER_SENSOR_PIN);
}

int SensorManager::getSoilPercentage() {
    int soilMoisture = readSoilMoisture();
    return map(soilMoisture, 0, 4095, 0, 100);
}

int SensorManager::getWaterPercentage() {
    int waterLevel = readWaterLevel();
    return map(waterLevel, 0, 4095, 0, 100);
}
