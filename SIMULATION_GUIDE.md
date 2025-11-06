# Simulation Mode Guide

## Overview
The GrowBox system now includes a **Simulation Mode** that allows you to manually set sensor values without having physical sensors connected. This is perfect for testing the web interface and verifying that all features work correctly.

## Enabling/Disabling Simulation Mode

### To Enable Simulation Mode (Default):
In `Config.h`, set:
```cpp
#define SIMULATION_MODE true
```

### To Disable Simulation Mode (Use Real Sensors):
In `Config.h`, set:
```cpp
#define SIMULATION_MODE false
```

## Using Simulation Mode

### 1. Access the Dashboard
- Connect to the GrowBox system
- Log in to access the dashboard
- You'll see a **yellow "Simulation Controls" panel** at the top

### 2. Set Sensor Values
The simulation panel allows you to set:

- **Temperature**: -40°C to 80°C (in 0.5°C steps)
  - Default: 25°C
  
- **Humidity**: 0% to 100% (in 1% steps)
  - Default: 50%
  
- **Soil Moisture**: 0% to 100% (in 1% steps)
  - Default: 30%
  
- **Water Level**: 0% to 100% (in 1% steps)
  - Default: 70%

### 3. Update Values
1. Enter your desired values in the input fields
2. Click the **"Update Simulation"** button
3. The page will automatically reload with the new values
4. All bars, meters, and the RGB LED will update accordingly

## Testing Features

### Test the RGB LED Color Changes:
The RGB LED changes color based on soil moisture:
- **Red** (255, 0, 0): Soil < 20% (Dry)
- **Orange** (255, 165, 0): Soil 20-60% (Moist)
- **Green** (0, 255, 0): Soil > 60% (Wet)

**Test Steps:**
1. Set Soil Moisture to 10% → RGB should show Red
2. Set Soil Moisture to 40% → RGB should show Orange
3. Set Soil Moisture to 80% → RGB should show Green

### Test Auto Pump Shutoff:
The pump automatically turns off when soil moisture > 80%

**Test Steps:**
1. Turn the pump ON manually
2. Set Soil Moisture to 85%
3. Click "Update Simulation"
4. The pump should automatically turn OFF

### Test All Bars and Meters:
**Test Steps:**
1. Set Temperature to 35°C → Temperature bar should fill to 35%
2. Set Humidity to 75% → Humidity bar should fill to 75%
3. Set Soil Moisture to 50% → Soil bar should fill to 50%
4. Set Water Level to 90% → Water bar should fill to 90%

### Test Edge Cases:
**Minimum Values:**
- Temperature: -40°C
- Humidity: 0%
- Soil: 0%
- Water: 0%

**Maximum Values:**
- Temperature: 80°C
- Humidity: 100%
- Soil: 100%
- Water: 100%

## Serial Monitor Output

When you update simulation values, the serial monitor shows:
```
Simulated Temperature set to: 25.00
Simulated Humidity set to: 50.00
Simulated Soil Moisture set to: 30
Simulated Water Level set to: 70
```

## Default Values

When the system starts in simulation mode, it uses these defaults:
- Temperature: 25°C
- Humidity: 50%
- Soil Moisture: 30%
- Water Level: 70%

## Switching Between Modes

### From Simulation to Real Sensors:
1. Set `SIMULATION_MODE false` in Config.h
2. Connect your physical sensors
3. Upload the firmware
4. The simulation panel will still be visible but won't affect anything
5. Real sensor values will be displayed

### From Real Sensors to Simulation:
1. Set `SIMULATION_MODE true` in Config.h
2. Upload the firmware
3. The simulation panel will control all sensor readings
4. Physical sensors (if connected) will be ignored

## Visual Design

The simulation panel has:
- **Yellow background** (#fff3cd) for easy identification
- **Orange border** (#ffc107) to stand out
- **Clear labels** with units
- **Wide button** for easy clicking
- **Responsive design** matching the rest of the interface

## Tips

1. **Use realistic values** to properly test your system
2. **Test extreme values** to see how the system handles them
3. **Check the RGB LED** to verify the color-coding works
4. **Monitor the serial output** to see when values change
5. **Test the auto-pump feature** by setting high soil moisture values

## Troubleshooting

**Simulation panel not visible:**
- Make sure you're logged in and viewing the dashboard
- Check that SIMULATION_MODE is set to true in Config.h
- Verify the page loaded completely

**Values not updating:**
- Check the browser console for JavaScript errors
- Verify you're connected to the WiFi
- Ensure the ESP32 is responding (check serial monitor)

**Page doesn't reload after update:**
- Manually refresh the page
- Check your network connection
- Clear browser cache if needed

## Example Test Sequence

1. **Start**: Default values (25°C, 50%, 30%, 70%)
2. **Test Dry Soil**: Set Soil to 15% → RGB turns Red
3. **Test Moist Soil**: Set Soil to 45% → RGB turns Orange
4. **Test Wet Soil**: Set Soil to 75% → RGB turns Green
5. **Test Auto Pump**: Turn pump ON, set Soil to 85% → Pump turns OFF
6. **Test High Temp**: Set Temperature to 60°C → Bar fills to 60%
7. **Test Low Water**: Set Water to 20% → Bar shows 20%

This confirms all features are working correctly! ✅
