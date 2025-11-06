#include "SensorManager.h"

SensorManager::SensorManager() : dht(DHTPIN, DHTTYPE),
    simulatedTemperature(25.0), simulatedHumidity(50.0),
    simulatedSoilPercentage(30), simulatedWaterPercentage(70) {
}

void SensorManager::begin() {
    dht.begin();
}

float SensorManager::readTemperature() {
#if SIMULATION_MODE
    return simulatedTemperature;
#else
    return dht.readTemperature();
#endif
}

float SensorManager::readHumidity() {
#if SIMULATION_MODE
    return simulatedHumidity;
#else
    return dht.readHumidity();
#endif
}

int SensorManager::readSoilMoisture() {
#if SIMULATION_MODE
    return map(simulatedSoilPercentage, 0, 100, 0, 4095);
#else
    return analogRead(SOIL_SENSOR_PIN);
#endif
}

int SensorManager::readWaterLevel() {
#if SIMULATION_MODE
    return map(simulatedWaterPercentage, 0, 100, 0, 4095);
#else
    return analogRead(WATER_SENSOR_PIN);
#endif
}

int SensorManager::getSoilPercentage() {
#if SIMULATION_MODE
    return simulatedSoilPercentage;
#else
    int soilMoisture = readSoilMoisture();
    return map(soilMoisture, 0, 4095, 0, 100);
#endif
}

int SensorManager::getWaterPercentage() {
#if SIMULATION_MODE
    return simulatedWaterPercentage;
#else
    int waterLevel = readWaterLevel();
    return map(waterLevel, 0, 4095, 0, 100);
#endif
}

void SensorManager::setSimulatedTemperature(float temp) {
    simulatedTemperature = constrain(temp, -40.0f, 80.0f);
}

void SensorManager::setSimulatedHumidity(float hum) {
    simulatedHumidity = constrain(hum, 0.0f, 100.0f);
}

void SensorManager::setSimulatedSoilPercentage(int soil) {
    simulatedSoilPercentage = constrain(soil, 0, 100);
}

void SensorManager::setSimulatedWaterPercentage(int water) {
    simulatedWaterPercentage = constrain(water, 0, 100);
}
