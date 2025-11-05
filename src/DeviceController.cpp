#include "DeviceController.h"

DeviceController::DeviceController() 
    : pumpState(false), growLedState(false), lastBrightness(0),
      redValue(0), greenValue(0), blueValue(0),
      lastDebounceTime(0), lastButtonState(HIGH) {
}

void DeviceController::begin() {
    // Initialize button as input with pull-up
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    // Initialize relay pins as outputs
    pinMode(PUMP_RELAY, OUTPUT);
    pinMode(GROWLED_RELAY, OUTPUT);
    digitalWrite(PUMP_RELAY, LOW);
    
    // Initialize RGB pins as outputs
    pinMode(RED_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);
    
    // Attach the RGB pins to PWM channels
    ledcSetup(RED_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(GREEN_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(BLUE_CHANNEL, PWM_FREQ, PWM_RESOLUTION);

    ledcAttachPin(RED_PIN, RED_CHANNEL);
    ledcAttachPin(GREEN_PIN, GREEN_CHANNEL);
    ledcAttachPin(BLUE_PIN, BLUE_CHANNEL);
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

void DeviceController::setRGBColor(int red, int green, int blue) {
    // Clamp values between 0 and 255
    redValue = constrain(red, 0, 255);
    greenValue = constrain(green, 0, 255);
    blueValue = constrain(blue, 0, 255);

    // Debug output
    Serial.printf("Setting RGB Color - R: %d, G: %d, B: %d\n", redValue, greenValue, blueValue);
    Serial.println();

    // Write PWM values to the channels
    ledcWrite(RED_CHANNEL, redValue);
    ledcWrite(GREEN_CHANNEL, greenValue);
    ledcWrite(BLUE_CHANNEL, blueValue);
}

void DeviceController::updateSoilMoistureColor(int soilPercentage) {
    if (soilPercentage < 20) {
        // Dry: Red
        setRGBColor(255, 0, 0);
    } else if (soilPercentage <= 60) {
        // Moist: Orange
        setRGBColor(255, 165, 0);
    } else {
        // Wet: Green
        setRGBColor(0, 255, 0);
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
