#include "SensorManager.h"
#include <Wire.h>

SensorManager::SensorManager() :
    simulatedTemperature(25.0), simulatedHumidity(50.0),
    simulatedSoilPercentage(30), simulatedWaterPercentage(70) {
}

void SensorManager::begin() {
    // Initialize I2C for SHT40
    Wire.begin(SHT40_SDA, SHT40_SCL);
    
    if (!sht4.begin()) {
        Serial.println("ERROR: SHT40 sensor not found!");
        Serial.println("Check wiring: SDA=GPIO21, SCL=GPIO22");
    } else {
        Serial.println("SHT40 sensor initialized successfully");
        sht4.setPrecision(SHT4X_HIGH_PRECISION);
        sht4.setHeater(SHT4X_NO_HEATER);
    }
    
    // Initialize sensor power pins as outputs and turn them OFF initially
    pinMode(SOIL_POWER_PIN, OUTPUT);
    pinMode(WATER_POWER_PIN, OUTPUT);
    digitalWrite(SOIL_POWER_PIN, LOW);
    digitalWrite(WATER_POWER_PIN, LOW);
    
    Serial.println("Sensor power control pins initialized (GPIO " + String(SOIL_POWER_PIN) + " & " + String(WATER_POWER_PIN) + ")");
}

float SensorManager::readTemperature() {
#if SIMULATION_MODE
    return simulatedTemperature;
#else
    sensors_event_t humidity, temp;
    sht4.getEvent(&humidity, &temp);
    return temp.temperature;
#endif
}

float SensorManager::readHumidity() {
#if SIMULATION_MODE
    return simulatedHumidity;
#else
    sensors_event_t humidity, temp;
    sht4.getEvent(&humidity, &temp);
    return humidity.relative_humidity;
#endif
}

int SensorManager::readSoilMoisture() {
#if SIMULATION_MODE
    return map(simulatedSoilPercentage, 0, 100, 0, 4095);
#else
    // Ensure both sensors are OFF before reading
    digitalWrite(SOIL_POWER_PIN, LOW);
    digitalWrite(WATER_POWER_PIN, LOW);
    delay(50);
    
    // Power ON the soil sensor
    digitalWrite(SOIL_POWER_PIN, HIGH);
    delay(150); // Wait for sensor to stabilize
    
    int soilValue = analogRead(SOIL_SENSOR_PIN);
    
    // Power OFF the soil sensor to prevent corrosion
    digitalWrite(SOIL_POWER_PIN, LOW);
    delay(50); // Give time for pin to fully discharge
    
    return soilValue;
#endif
}

int SensorManager::readWaterLevel() {
#if SIMULATION_MODE
    return map(simulatedWaterPercentage, 0, 100, 0, 4095);
#else
    // Ensure both sensors are OFF before reading
    digitalWrite(SOIL_POWER_PIN, LOW);
    digitalWrite(WATER_POWER_PIN, LOW);
    delay(50);
    
    // Power ON the water level sensor
    digitalWrite(WATER_POWER_PIN, HIGH);
    Serial.printf("Water power pin %d set to HIGH\n", WATER_POWER_PIN);
    delay(150); // Wait for sensor to stabilize
    
    int waterValue = analogRead(WATER_SENSOR_PIN);
    
    // Power OFF the water level sensor to prevent corrosion
    digitalWrite(WATER_POWER_PIN, LOW);
    delay(50); // Give time for pin to fully discharge
    Serial.printf("Water sensor read: %d (pin %d)\n", waterValue, WATER_SENSOR_PIN);
    
    return waterValue;
#endif
}

int SensorManager::getSoilPercentage() {
#if SIMULATION_MODE
    return simulatedSoilPercentage;
#else
    int soilMoisture = readSoilMoisture();
    // Invert and map: dry (high value) = 0%, wet (low value) = 100%
    int percentage = map(soilMoisture, SOIL_DRY_VALUE, SOIL_WET_VALUE, 0, 100);
    percentage = constrain(percentage, 0, 100);
    Serial.printf("Soil: raw=%d, percentage=%d%%\n", soilMoisture, percentage);
    return percentage;
#endif
}

int SensorManager::getWaterPercentage() {
#if SIMULATION_MODE
    return simulatedWaterPercentage;
#else
    int waterLevel = readWaterLevel();
    // Map normally: empty (low value) = 0%, full (high value) = 100%
    int percentage = map(waterLevel, WATER_EMPTY_VALUE, WATER_FULL_VALUE, 0, 100);
    percentage = constrain(percentage, 0, 100);
    Serial.printf("Water: raw=%d, percentage=%d%%\n", waterLevel, percentage);
    return percentage;
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
