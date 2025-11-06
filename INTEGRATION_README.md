# GrowBox Emulation - Integrated Authentication System

## Overview
This project combines a GrowBox monitoring and control system with a WiFi setup and authentication system. Users must first configure WiFi and log in before accessing the GrowBox dashboard.

## Project Structure

```
src/
├── main.cpp              - Main application entry point
├── Config.h              - Pin definitions and constants
├── AuthManager.h/cpp     - WiFi setup and authentication
├── SensorManager.h/cpp   - DHT22, soil, and water sensors
├── DeviceController.h/cpp - Pump, LEDs, and RGB LED control
├── WebServerManager.h/cpp - Web server and route handling
└── WebPage.h/cpp         - GrowBox dashboard HTML
```

## Features

### 1. WiFi Setup & Authentication
- **Access Point Mode**: ESP32 creates "GrowBox-Setup" WiFi network
- **Network Scanning**: Lists available WiFi networks
- **Secure Login**: Username and password authentication
- **Dual Mode**: Works as both Access Point and Station

### 2. GrowBox Dashboard
After authentication, users access the full GrowBox dashboard with:
- **Temperature & Humidity** monitoring (DHT22 sensor)
- **Soil Moisture** monitoring with auto-pump control
- **Water Level** monitoring
- **Pump Control** (manual toggle + auto-off at 80% soil moisture)
- **Grow LED Control** with brightness adjustment
- **RGB LED** indicating soil moisture status:
  - Red: Dry (<20%)
  - Orange: Moist (20-60%)
  - Green: Wet (>60%)
- **Physical Button** for manual pump control

## Getting Started

### 1. Initial Setup
1. Upload the firmware to your ESP32
2. The ESP32 will create a WiFi Access Point named **"GrowBox-Setup"**
3. Password: **growbox123**

### 2. Connect to the System
1. Connect your device to "GrowBox-Setup" WiFi
2. Open a web browser and go to: **http://192.168.4.1**
3. You'll see the login page

### 3. Configure WiFi & Login
1. Select your WiFi network from the dropdown
2. Enter your WiFi password
3. Enter login credentials:
   - **Default Username**: admin
   - **Default Password**: password123
4. Click "Connect & Login"

### 4. Access the Dashboard
Once connected, you'll be redirected to the GrowBox dashboard where you can:
- Monitor all sensors
- Control the pump and grow LED
- Adjust LED brightness
- View real-time updates

## Default Credentials

**WiFi Access Point:**
- SSID: `GrowBox-Setup`
- Password: `growbox123`

**Login:**
- Username: `admin`
- Password: `password123`

## Pin Configuration

### Sensors
- DHT22: GPIO 4
- Soil Moisture: GPIO 33 (Analog)
- Water Level: GPIO 34 (Analog)
- Push Button: GPIO 32

### Actuators
- Pump Relay: GPIO 26
- Grow LED Relay: GPIO 27
- Grow LED PWM: GPIO 5

### RGB LED
- Red: GPIO 23
- Green: GPIO 22
- Blue: GPIO 21

## Auto Features

1. **Auto Pump Shutoff**: Pump automatically turns off when soil moisture > 80%
2. **Auto Color Indication**: RGB LED changes color based on soil moisture
3. **WiFi Reconnect**: Automatically attempts to reconnect if WiFi drops

## URL Routes

- `/` - Login page (if not authenticated) or redirect to dashboard
- `/connect` - Handle WiFi connection and authentication
- `/scan-networks` - Refresh available networks
- `/dashboard` - Main GrowBox control panel (requires authentication)
- `/toggle/1` - Toggle pump
- `/toggle/2` - Toggle grow LED
- `/brightness/{value}` - Set grow LED brightness (0-100)

## Customization

### Change Login Credentials
Edit in `main.cpp`:
```cpp
AuthManager auth("your_username", "your_password");
```

### Change Access Point Settings
Edit in `AuthManager.cpp`:
```cpp
WiFi.softAP("YourAPName", "YourPassword");
```

### Modify Pin Assignments
Edit in `Config.h`

## Troubleshooting

**Can't connect to Access Point:**
- Make sure the ESP32 is powered on
- Check that "GrowBox-Setup" appears in WiFi list
- Try restarting the ESP32

**Login fails:**
- Verify username and password (case-sensitive)
- Check serial monitor for error messages

**Dashboard not loading:**
- Ensure WiFi connection was successful
- Check that you're connected to the configured WiFi network
- Try accessing via IP address shown after WiFi connection

**Sensors not reading:**
- Verify sensor connections
- Check pin assignments in Config.h
- Monitor serial output for sensor errors

## Serial Monitor Output

The serial monitor (115200 baud) provides:
- Startup messages
- WiFi connection status
- IP addresses (AP and Station)
- Button press events
- Pump state changes
- RGB color changes
- HTTP request logs

## Memory Usage

- **RAM**: ~14% (46KB used)
- **Flash**: ~64% (840KB used)

## Dependencies

- Arduino Framework for ESP32
- WiFi library
- WebServer library
- DHT sensor library
- Adafruit Unified Sensor

## License

See LICENSE file in the project root.
