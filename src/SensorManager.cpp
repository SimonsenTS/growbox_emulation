#include "SensorManager.h"
#include <Wire.h>

SensorManager::SensorManager() :
    simulatedTemperature(25.0), simulatedHumidity(50.0),
    simulatedSoilPercentage(30), simulatedWaterPercentage(70) {
}

void SensorManager::begin() {
    // Initialize I2C for SHT40 and Grove Water Sensor
    Wire.begin(SHT40_SDA, SHT40_SCL);
    Wire.setClock(100000);  // Set I2C to 100kHz for better stability
    
    if (!sht4.begin()) {
        Serial.println("ERROR: SHT40 sensor not found!");
        Serial.printf("Check wiring: SDA=GPIO%d, SCL=GPIO%d\n", SHT40_SDA, SHT40_SCL);
    } else {
        Serial.println("SHT40 sensor initialized successfully");
        sht4.setPrecision(SHT4X_HIGH_PRECISION);
        sht4.setHeater(SHT4X_NO_HEATER);
    }
    
    // Check if Grove Water Level Sensor is present
    Serial.println("Checking for Grove Water Level Sensor...");
    Wire.beginTransmission(WATER_LEVEL_I2C_ADDR_LOW);
    byte error_low = Wire.endTransmission();
    Wire.beginTransmission(WATER_LEVEL_I2C_ADDR_HIGH);
    byte error_high = Wire.endTransmission();
    
    if (error_low == 0 && error_high == 0) {
        Serial.println("Grove Water Level Sensor found (Addresses: 0x77, 0x78)");
    } else {
        Serial.println("WARNING: Grove Water Level Sensor NOT detected!");
        Serial.printf("I2C scan result - 0x77: %s, 0x78: %s\n", 
                     error_low == 0 ? "OK" : "FAIL",
                     error_high == 0 ? "OK" : "FAIL");
    }
    
    // Initialize sensor power pins as outputs and turn them OFF initially
    pinMode(SOIL_POWER_PIN, OUTPUT);
    digitalWrite(SOIL_POWER_PIN, LOW);
    
    Serial.printf("Sensor power control pin initialized (Soil: GPIO %d)\n", SOIL_POWER_PIN);
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

void SensorManager::getLow8SectionValue(unsigned char* low_data) {
    memset(low_data, 0, 8);
    
    // Check if device responds before requesting data
    Wire.beginTransmission(WATER_LEVEL_I2C_ADDR_LOW);
    if (Wire.endTransmission() != 0) {
        return;  // Device not responding, return zeros
    }
    
    Wire.requestFrom(WATER_LEVEL_I2C_ADDR_LOW, 8);
    
    unsigned long timeout = millis() + 100;
    while (Wire.available() < 8 && millis() < timeout);
    
    int available = Wire.available();
    
    if (available >= 8) {
        for (int i = 0; i < 8; i++) {
            low_data[i] = Wire.read();
        }
    } else if (available > 0) {
        // Read whatever is available
        for (int i = 0; i < available && i < 8; i++) {
            low_data[i] = Wire.read();
        }
    }
}

void SensorManager::getHigh12SectionValue(unsigned char* high_data) {
    memset(high_data, 0, 12);
    
    // Check if device responds before requesting data
    Wire.beginTransmission(WATER_LEVEL_I2C_ADDR_HIGH);
    if (Wire.endTransmission() != 0) {
        return;  // Device not responding, return zeros
    }
    
    Wire.requestFrom(WATER_LEVEL_I2C_ADDR_HIGH, 12);
    
    unsigned long timeout = millis() + 100;
    while (Wire.available() < 12 && millis() < timeout);
    
    int available = Wire.available();
    
    if (available >= 12) {
        for (int i = 0; i < 12; i++) {
            high_data[i] = Wire.read();
        }
    } else if (available > 0) {
        // Read whatever is available
        for (int i = 0; i < available && i < 12; i++) {
            high_data[i] = Wire.read();
        }
    }
}

int SensorManager::readWaterLevel() {
#if SIMULATION_MODE
    return simulatedWaterPercentage;
#else
    unsigned char low_data[8] = {0};
    unsigned char high_data[12] = {0};
    uint32_t touch_val = 0;
    uint8_t trig_section = 0;
    
    // Read data from both I2C addresses
    getLow8SectionValue(low_data);
    getHigh12SectionValue(high_data);
    
    // Count triggered sections (capacitive touch detection)
    for (int i = 0; i < 8; i++) {
        if (low_data[i] > WATER_LEVEL_THRESHOLD) {
            touch_val |= 1 << i;
        }
    }
    
    for (int i = 0; i < 12; i++) {
        if (high_data[i] > WATER_LEVEL_THRESHOLD) {
            touch_val |= (uint32_t)1 << (8 + i);
        }
    }
    
    // Count consecutive triggered sections from bottom
    while (touch_val & 0x01) {
        trig_section++;
        touch_val >>= 1;
    }
    
    return trig_section;
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
    int sections = readWaterLevel();
    // Convert sections (0-20) to percentage (0-100%)
    // Each section represents 5% (20 sections * 5% = 100%)
    int percentage = (sections * 100) / WATER_LEVEL_MAX_SECTIONS;
    percentage = constrain(percentage, 0, 100);
    Serial.printf("Water: sections=%d, percentage=%d%%\n", sections, percentage);
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
