#include "DeviceController.h"

DeviceController::DeviceController() 
    : pumpState(false), growLedState(false), lastBrightness(0), savedBrightness(50),
      rgbLedsEnabled(true),
      soilLED(NUM_LEDS, SOIL_LED_PIN, NEO_GRB + NEO_KHZ800),
      waterLED(NUM_LEDS, WATER_LED_PIN, NEO_GRB + NEO_KHZ800),
      soilColor(0), waterColor(0),
      lastDebounceTime(0), lastButtonState(HIGH) {
}

void DeviceController::begin() {
    // Initialize button as input with pull-up
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    // Initialize relay pins as outputs
    pinMode(PUMP_RELAY, OUTPUT);
    pinMode(GROWLED_RELAY, OUTPUT);
    digitalWrite(PUMP_RELAY, LOW);
    
    // Initialize WS2812B RGB LEDs
    soilLED.begin();
    soilLED.setBrightness(255);  // Full brightness
    soilLED.clear();
    soilLED.show();
    
    waterLED.begin();
    waterLED.setBrightness(255);  // Full brightness
    waterLED.clear();
    waterLED.show();
    
    Serial.println("WS2812B LEDs initialized (Soil: GPIO " + String(SOIL_LED_PIN) + ", Water: GPIO " + String(WATER_LED_PIN) + ")");
}

void DeviceController::setPumpState(bool state) {
    pumpState = state;
    digitalWrite(PUMP_RELAY, pumpState);
    Serial.printf("Pump relay set to: %s (GPIO %d = %d)\n", pumpState ? "ON" : "OFF", PUMP_RELAY, pumpState);
}

void DeviceController::togglePump() {
    Serial.printf("Toggle pump called - current state: %s\n", pumpState ? "ON" : "OFF");
    setPumpState(!pumpState);
    Serial.printf("Pump toggled to: %s\n", pumpState ? "ON" : "OFF");
}

void DeviceController::setGrowLedState(bool state) {
    growLedState = state;
    digitalWrite(GROWLED_RELAY, growLedState);
    
    if (growLedState) {
        // Turning ON: restore previous brightness
        updateGrowLEDBrightness(savedBrightness);
        Serial.printf("Grow LED turned ON - Restored brightness: %d%%\n", savedBrightness);
    } else {
        // Turning OFF: save current brightness and set to 0
        savedBrightness = lastBrightness;
        updateGrowLEDBrightness(0);
        Serial.printf("Grow LED turned OFF - Saved brightness: %d%%\n", savedBrightness);
    }
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
    red = constrain(red, 0, 255);
    green = constrain(green, 0, 255);
    blue = constrain(blue, 0, 255);

    // Store color as 32-bit value (0x00RRGGBB)
    soilColor = ((uint32_t)red << 16) | ((uint32_t)green << 8) | blue;

    // Debug output
    Serial.printf("Setting Soil Moisture LED (WS2812B) - R: %d, G: %d, B: %d\n", red, green, blue);

    // Only update LED if enabled
    if (rgbLedsEnabled) {
        soilLED.setPixelColor(0, soilLED.Color(red, green, blue));
        soilLED.show();
    } else {
        soilLED.clear();
        soilLED.show();
    }
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
    red = constrain(red, 0, 255);
    green = constrain(green, 0, 255);
    blue = constrain(blue, 0, 255);

    // Store color as 32-bit value (0x00RRGGBB)
    waterColor = ((uint32_t)red << 16) | ((uint32_t)green << 8) | blue;

    // Debug output
    Serial.printf("Setting Water Level LED (WS2812B) - R: %d, G: %d, B: %d\n", red, green, blue);

    // Force ON if water is critical (red), otherwise respect user setting
    bool isCritical = (red == 255 && green == 0 && blue == 0);
    if (isCritical || rgbLedsEnabled) {
        waterLED.setPixelColor(0, waterLED.Color(red, green, blue));
        waterLED.show();
        if (isCritical && !rgbLedsEnabled) {
            Serial.println(">>> WATER CRITICAL: LED forced ON despite user preference!");
        }
    } else {
        waterLED.clear();
        waterLED.show();
    }
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

void DeviceController::setRGBLedsEnabled(bool enabled) {
    rgbLedsEnabled = enabled;
    Serial.printf("RGB LEDs (WS2812B) %s\n", enabled ? "ENABLED" : "DISABLED");
    
    if (!enabled) {
        // Turn off both LEDs
        soilLED.clear();
        soilLED.show();
        waterLED.clear();
        waterLED.show();
    } else {
        // Re-apply stored colors
        soilLED.setPixelColor(0, soilColor);
        soilLED.show();
        waterLED.setPixelColor(0, waterColor);
        waterLED.show();
    }
}

void DeviceController::toggleRGBLeds() {
    setRGBLedsEnabled(!rgbLedsEnabled);
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
