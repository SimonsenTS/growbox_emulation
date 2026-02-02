#ifndef DEVICECONTROLLER_H
#define DEVICECONTROLLER_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "Config.h"

class DeviceController {
private:
    bool pumpState;
    bool growLedState;
    int lastBrightness;
    int savedBrightness;  // Saves brightness level before turning OFF
    
    // WS2812B RGB LEDs
    Adafruit_NeoPixel soilLED;
    Adafruit_NeoPixel waterLED;
    
    // Current LED color values for tracking
    uint32_t soilColor;
    uint32_t waterColor;
    
    unsigned long lastDebounceTime;
    bool lastButtonState;
    
public:
    DeviceController();
    void begin();
    
    // Pump control
    void setPumpState(bool state);
    bool getPumpState() const { return pumpState; }
    void togglePump();
    
    // Grow LED control
    void setGrowLedState(bool state);
    bool getGrowLedState() const { return growLedState; }
    void toggleGrowLed();
    void updateGrowLEDBrightness(int brightness);
    int getBrightness() const { return lastBrightness; }
    
    // Soil Moisture RGB LED control (WS2812B)
    void setSoilRGBColor(int red, int green, int blue);
    int getSoilRedValue() const { return (soilColor >> 16) & 0xFF; }
    int getSoilGreenValue() const { return (soilColor >> 8) & 0xFF; }
    int getSoilBlueValue() const { return soilColor & 0xFF; }
    void updateSoilMoistureColor(int soilPercentage);
    
    // Water Level RGB LED control (WS2812B)
    void setWaterRGBColor(int red, int green, int blue);
    int getWaterRedValue() const { return (waterColor >> 16) & 0xFF; }
    int getWaterGreenValue() const { return (waterColor >> 8) & 0xFF; }
    int getWaterBlueValue() const { return waterColor & 0xFF; }
    void updateWaterLevelColor(int waterPercentage);
    
    // Button handling
    bool checkButton();
};

#endif // DEVICECONTROLLER_H
