#ifndef DEVICECONTROLLER_H
#define DEVICECONTROLLER_H

#include <Arduino.h>
#include "Config.h"

class DeviceController {
private:
    bool pumpState;
    bool growLedState;
    int lastBrightness;
    
    int redValue;
    int greenValue;
    int blueValue;
    
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
    
    // RGB LED control
    void setRGBColor(int red, int green, int blue);
    int getRedValue() const { return redValue; }
    int getGreenValue() const { return greenValue; }
    int getBlueValue() const { return blueValue; }
    void updateSoilMoistureColor(int soilPercentage);
    
    // Button handling
    bool checkButton();
};

#endif // DEVICECONTROLLER_H
