#ifndef DEVICECONTROLLER_H
#define DEVICECONTROLLER_H

#include <Arduino.h>
#include "Config.h"

class DeviceController {
private:
    bool pumpState;
    bool growLedState;
    int lastBrightness;
    
    // Soil Moisture RGB LED values
    int soilRedValue;
    int soilGreenValue;
    int soilBlueValue;
    
    // Water Level RGB LED values
    int waterRedValue;
    int waterGreenValue;
    int waterBlueValue;
    
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
    
    // Soil Moisture RGB LED control
    void setSoilRGBColor(int red, int green, int blue);
    int getSoilRedValue() const { return soilRedValue; }
    int getSoilGreenValue() const { return soilGreenValue; }
    int getSoilBlueValue() const { return soilBlueValue; }
    void updateSoilMoistureColor(int soilPercentage);
    
    // Water Level RGB LED control
    void setWaterRGBColor(int red, int green, int blue);
    int getWaterRedValue() const { return waterRedValue; }
    int getWaterGreenValue() const { return waterGreenValue; }
    int getWaterBlueValue() const { return waterBlueValue; }
    void updateWaterLevelColor(int waterPercentage);
    
    // Button handling
    bool checkButton();
};

#endif // DEVICECONTROLLER_H
