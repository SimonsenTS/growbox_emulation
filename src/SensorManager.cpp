#include "SensorManager.h"
#include <Wire.h>
#include <ens210.h>

SensorManager::SensorManager() :
    simulatedTemperature(25.0), simulatedHumidity(50.0),
    simulatedSoilPercentage(30), simulatedWaterPercentage(70) {
}

void SensorManager::begin() {
    // --- I2C Bus Recovery ---
    // If ENS210 or any device is holding SDA low after power-on/reset,
    // manually clock SCL 9 times to force the device to release SDA.
    pinMode(SHT40_SCL, OUTPUT);
    pinMode(SHT40_SDA, INPUT_PULLUP);
    for (int i = 0; i < 9; i++) {
        digitalWrite(SHT40_SCL, HIGH); delayMicroseconds(5);
        digitalWrite(SHT40_SCL, LOW);  delayMicroseconds(5);
    }
    // Send a STOP condition (SDA low→high while SCL high)
    pinMode(SHT40_SDA, OUTPUT);
    digitalWrite(SHT40_SDA, LOW);  delayMicroseconds(5);
    digitalWrite(SHT40_SCL, HIGH); delayMicroseconds(5);
    digitalWrite(SHT40_SDA, HIGH); delayMicroseconds(5);
    // Pins back to input before Wire takes over
    pinMode(SHT40_SCL, INPUT);
    pinMode(SHT40_SDA, INPUT);
    delay(10);

    // --- Active sensor: ENS210 ---
    // Wire must be initialised with our custom pins BEFORE ens210.begin(),
    // because the library's _i2c_init() calls Wire.begin() with no arguments.
    Wire.begin(SHT40_SDA, SHT40_SCL);   // SDA=GPIO18, SCL=GPIO17
    Wire.setClock(100000);
    Wire.setTimeOut(50);
    delay(10);

    ens210Found = ens210.begin();
    if (!ens210Found) {
        Serial.println("ERROR: ENS210 not found! Check SDA=GPIO18, SCL=GPIO17");
    } else {
        Serial.println("ENS210 OK");
    }

    // Check Grove Water Level Sensor
    Wire.beginTransmission(WATER_LEVEL_I2C_ADDR_LOW);
    byte error_low = Wire.endTransmission();
    Wire.beginTransmission(WATER_LEVEL_I2C_ADDR_HIGH);
    byte error_high = Wire.endTransmission();
    if (error_low == 0 && error_high == 0) {
        Serial.println("Grove Water Level Sensor OK (0x77, 0x78)");
    } else {
        Serial.printf("WARNING: Grove Water Level - 0x77: %s, 0x78: %s\n",
                     error_low == 0 ? "OK" : "MISSING",
                     error_high == 0 ? "OK" : "MISSING");
    }

    // Initialize sensor power pins as outputs and turn them OFF initially
    pinMode(SOIL_POWER_PIN, OUTPUT);
    digitalWrite(SOIL_POWER_PIN, LOW);
    pinMode(WATER_POWER_PIN, OUTPUT);
    digitalWrite(WATER_POWER_PIN, LOW);

    Serial.printf("Sensor power pins initialized (Soil: GPIO%d, Water: GPIO%d)\n", SOIL_POWER_PIN, WATER_POWER_PIN);
}

void SensorManager::measureENS210() {
#if !SIMULATION_MODE
    if (ens210Found)
        ens210.measure(&_last_t_data, &_last_t_status, &_last_h_data, &_last_h_status);
#endif
}

float SensorManager::readTemperature() {
#if SIMULATION_MODE
    return simulatedTemperature;
#else
    if (!ens210Found || _last_t_status != ENS210_STATUS_OK) return -999.0f;
    return ens210.toCelsius(_last_t_data, 100) / 100.0f;
#endif
}

float SensorManager::readHumidity() {
#if SIMULATION_MODE
    return simulatedHumidity;
#else
    if (!ens210Found || _last_h_status != ENS210_STATUS_OK) return -999.0f;
    return ens210.toPercentageH(_last_h_data, 100) / 100.0f;
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
