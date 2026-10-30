#include "SensorManager.h"
#include "ControlPolicy.h"
#include <math.h>
#include <Wire.h>
#include <Adafruit_SHT4x.h>

SensorManager::SensorManager() :
    simulatedTemperature(25.0), simulatedHumidity(50.0),
    simulatedSoilPercentage(30), simulatedWaterPercentage(70) {
}

void SensorManager::begin() {
    pinMode(SOIL_POWER_PIN, OUTPUT);
    digitalWrite(SOIL_POWER_PIN, LOW);
#if SIMULATION_MODE
    return; // simulation never initializes or reads physical sensors
#endif
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

    // --- Wire bus: SHT40 + Grove Water Level ---
    Wire.begin(SHT40_SDA, SHT40_SCL);   // SDA=GPIO18, SCL=GPIO17
    Wire.setClock(400000);
    Wire.setTimeOut(50);
    delay(10);

    sht4Found = sht4.begin(&Wire);
    if (!sht4Found) {
        Serial.println("ERROR: SHT40 not found! Check SDA=GPIO18, SCL=GPIO17");
    } else {
        sht4.setPrecision(SHT4X_HIGH_PRECISION);
        sht4.setHeater(SHT4X_NO_HEATER);
        Serial.println("SHT40 OK");
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

#if ENABLE_LUX_SENSOR
    lightFound = light.begin(&Wire);
    if (lightFound) {
        light.setGain(LTR3XX_GAIN_1);
        light.setIntegrationTime(LTR3XX_INTEGTIME_100);
        light.setMeasurementRate(LTR3XX_MEASRATE_500);
    }
    Serial.println(lightFound ? "LTR-303 OK" : "LTR-303 absent: lux unavailable");
#endif
    // Initialize sensor power pins as outputs and turn them OFF initially
    pinMode(SOIL_POWER_PIN, OUTPUT);
    digitalWrite(SOIL_POWER_PIN, LOW);
    pinMode(WATER_POWER_PIN, OUTPUT);
    digitalWrite(WATER_POWER_PIN, LOW);
    Serial.printf("Sensor power pins initialized (Soil: GPIO%d, Water: GPIO%d)\n", SOIL_POWER_PIN, WATER_POWER_PIN);
}

void SensorManager::measureSHT40() {
#if !SIMULATION_MODE
    if (sht4Found) {
        sensors_event_t humidity, temp;
        if (sht4.getEvent(&humidity, &temp) && isfinite(temp.temperature) &&
            isfinite(humidity.relative_humidity)) {
            _lastTemp = temp.temperature;
            _lastHumidity = humidity.relative_humidity;
        } else {
            _lastTemp = _lastHumidity = -999.0f;
        }
    }
#endif
}

float SensorManager::readTemperature() {
#if SIMULATION_MODE
    return simulatedTemperature;
#else
    if (!sht4Found) return -999.0f;
    return _lastTemp;
#endif
}

float SensorManager::readHumidity() {
#if SIMULATION_MODE
    return simulatedHumidity;
#else
    if (!sht4Found) return -999.0f;
    return _lastHumidity;
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

// Wire.requestFrom is bounded by Wire.setTimeOut(50). No busy-wait loops.
bool SensorManager::getLow8SectionValue(unsigned char* data) {
    if (Wire.requestFrom(uint8_t(WATER_LEVEL_I2C_ADDR_LOW), uint8_t(8)) != 8) {
        while (Wire.available()) Wire.read();
        return false;
    }
    for (int i = 0; i < 8; ++i) data[i] = Wire.read();
    return true;
}
bool SensorManager::getHigh12SectionValue(unsigned char* data) {
    if (Wire.requestFrom(uint8_t(WATER_LEVEL_I2C_ADDR_HIGH), uint8_t(12)) != 12) {
        while (Wire.available()) Wire.read();
        return false;
    }
    for (int i = 0; i < 12; ++i) data[i] = Wire.read();
    return true;
}

int SensorManager::readWaterLevel() {
#if SIMULATION_MODE
    return simulatedWaterPercentage;
#else
    unsigned char low_data[8] = {0};
    unsigned char high_data[12] = {0};
    uint8_t trig_section = 0;
    
    // Partial/missing readings are invalid, rather than a plausible percentage.
    if (!getLow8SectionValue(low_data) || !getHigh12SectionValue(high_data)) return -1;

    // Count all triggered sections (not consecutive from bottom - sensor byte order may vary)
    for (int i = 0; i < 8; i++) {
        if (low_data[i] > WATER_LEVEL_THRESHOLD) trig_section++;
    }
    for (int i = 0; i < 12; i++) {
        if (high_data[i] > WATER_LEVEL_THRESHOLD) trig_section++;
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
    if (sections < 0) return -1;
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

float SensorManager::readLux() {
#if SIMULATION_MODE
    return simulatedLux;
#elif ENABLE_LUX_SENSOR
    uint16_t ch0, ch1;
    if (!lightFound || !light.newDataAvailable() || !light.readBothChannels(ch0, ch1)) return -1;
    return ControlPolicy::lux(ch0, ch1);
#else
    return -1; // PCB v1 has no lux sensor
#endif
}
void SensorManager::setSimulatedLux(float lux) {
    simulatedLux = constrain(lux, 0.0f, 100000.0f);
}
