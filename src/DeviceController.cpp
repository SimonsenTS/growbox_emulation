#include "DeviceController.h"

DeviceController::DeviceController() 
    : pumpState(false), growLedState(false), lastBrightness(0),
      soilRedValue(0), soilGreenValue(0), soilBlueValue(0),
      waterRedValue(0), waterGreenValue(0), waterBlueValue(0),
      lastDebounceTime(0), lastButtonState(HIGH) {
}

void DeviceController::begin() {
    // Initialize button as input with pull-up
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    // Initialize relay pins as outputs
    pinMode(PUMP_RELAY, OUTPUT);
    pinMode(GROWLED_RELAY, OUTPUT);
    digitalWrite(PUMP_RELAY, LOW);
    
    // Initialize Soil Moisture RGB LED pins as outputs
    pinMode(SOIL_RED_PIN, OUTPUT);
    pinMode(SOIL_GREEN_PIN, OUTPUT);
    pinMode(SOIL_BLUE_PIN, OUTPUT);
    
    // Initialize Water Level RGB LED pins as outputs
    pinMode(WATER_RED_PIN, OUTPUT);
    pinMode(WATER_GREEN_PIN, OUTPUT);
    pinMode(WATER_BLUE_PIN, OUTPUT);
    
    // Setup PWM channels for Soil Moisture RGB LED
    ledcSetup(SOIL_RED_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(SOIL_GREEN_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(SOIL_BLUE_CHANNEL, PWM_FREQ, PWM_RESOLUTION);

    ledcAttachPin(SOIL_RED_PIN, SOIL_RED_CHANNEL);
    ledcAttachPin(SOIL_GREEN_PIN, SOIL_GREEN_CHANNEL);
    ledcAttachPin(SOIL_BLUE_PIN, SOIL_BLUE_CHANNEL);
    
    // Setup PWM channels for Water Level RGB LED
    ledcSetup(WATER_RED_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(WATER_GREEN_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(WATER_BLUE_CHANNEL, PWM_FREQ, PWM_RESOLUTION);

    ledcAttachPin(WATER_RED_PIN, WATER_RED_CHANNEL);
    ledcAttachPin(WATER_GREEN_PIN, WATER_GREEN_CHANNEL);
    ledcAttachPin(WATER_BLUE_PIN, WATER_BLUE_CHANNEL);
}

void DeviceController::setPumpState(bool state) {
    pumpState = state;
    digitalWrite(PUMP_RELAY, pumpState);
}

void DeviceController::togglePump() {
    setPumpState(!pumpState);
}

void DeviceController::setGrowLedState(bool state) {
    growLedState = state;
    digitalWrite(GROWLED_RELAY, growLedState);
}

void DeviceController::toggleGrowLed() {
    setGrowLedState(!growLedState);
}

void DeviceController::updateGrowLEDBrightness(int brightness) {
    lastBrightness = brightness;
    int pwmValue = map(brightness, 0, 100, 0, 255);
    analogWrite(GROWLED_PWM, pwmValue);
    Serial.print("Brightness value ");
    Serial.println(brightness);
}

void DeviceController::setSoilRGBColor(int red, int green, int blue) {
    // Clamp values between 0 and 255
    soilRedValue = constrain(red, 0, 255);
    soilGreenValue = constrain(green, 0, 255);
    soilBlueValue = constrain(blue, 0, 255);

    // Debug output
    Serial.printf("Setting Soil Moisture LED - R: %d, G: %d, B: %d\n", soilRedValue, soilGreenValue, soilBlueValue);

    // Write PWM values to the channels
    ledcWrite(SOIL_RED_CHANNEL, soilRedValue);
    ledcWrite(SOIL_GREEN_CHANNEL, soilGreenValue);
    ledcWrite(SOIL_BLUE_CHANNEL, soilBlueValue);
}

void DeviceController::updateSoilMoistureColor(int soilPercentage) {
    if (soilPercentage < 20) {
        // Dry: Red
        setSoilRGBColor(255, 0, 0);
    } else if (soilPercentage <= 60) {
        // Moist: Orange
        setSoilRGBColor(255, 165, 0);
    } else {
        // Wet: Green
        setSoilRGBColor(0, 255, 0);
    }
}

void DeviceController::setWaterRGBColor(int red, int green, int blue) {
    // Clamp values between 0 and 255
    waterRedValue = constrain(red, 0, 255);
    waterGreenValue = constrain(green, 0, 255);
    waterBlueValue = constrain(blue, 0, 255);

    // Debug output
    Serial.printf("Setting Water Level LED - R: %d, G: %d, B: %d\n", waterRedValue, waterGreenValue, waterBlueValue);

    // Write PWM values to the channels
    ledcWrite(WATER_RED_CHANNEL, waterRedValue);
    ledcWrite(WATER_GREEN_CHANNEL, waterGreenValue);
    ledcWrite(WATER_BLUE_CHANNEL, waterBlueValue);
}

void DeviceController::updateWaterLevelColor(int waterPercentage) {
    if (waterPercentage < 10) {
        // Low: Red
        setWaterRGBColor(255, 0, 0);
    } else if (waterPercentage <= 50) {
        // Medium: Orange
        setWaterRGBColor(255, 165, 0);
    } else {
        // High: Green
        setWaterRGBColor(0, 255, 0);
    }
}

bool DeviceController::checkButton() {
    bool currentButtonState = digitalRead(BUTTON_PIN);

    if (currentButtonState == LOW && lastButtonState == HIGH && 
        (millis() - lastDebounceTime > DEBOUNCE_DELAY)) {
        lastDebounceTime = millis();
        lastButtonState = currentButtonState;
        return true;
    }

    lastButtonState = currentButtonState;
    return false;
}
