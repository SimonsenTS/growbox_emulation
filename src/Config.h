#ifndef CONFIG_H
#define CONFIG_H

// WiFi configuration is now handled through the web interface
// No need for hardcoded credentials - users configure via GrowBox-Setup AP

// DHT sensor configuration
#define DHTPIN 4
#define DHTTYPE DHT11

// Sensor pins
#define SOIL_SENSOR_PIN 33
#define WATER_SENSOR_PIN 34
#define BUTTON_PIN 32

// PWM configuration
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8

// PWM Channels for Soil Moisture RGB LED
#define SOIL_RED_CHANNEL 0
#define SOIL_GREEN_CHANNEL 1
#define SOIL_BLUE_CHANNEL 2

// PWM Channels for Water Level RGB LED
#define WATER_RED_CHANNEL 3
#define WATER_GREEN_CHANNEL 4
#define WATER_BLUE_CHANNEL 5

// Relay and LED pins
#define PUMP_RELAY 26
#define GROWLED_RELAY 27
#define GROWLED_PWM 5

// Soil Moisture RGB LED pins
#define SOIL_RED_PIN 23
#define SOIL_GREEN_PIN 22
#define SOIL_BLUE_PIN 21

// Water Level RGB LED pins
#define WATER_RED_PIN 19
#define WATER_GREEN_PIN 18
#define WATER_BLUE_PIN 17

// Time configuration
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 0
#define DAYLIGHT_OFFSET_SEC 3600

// Button debounce
#define DEBOUNCE_DELAY 200

// Simulation mode - set to true to enable manual sensor input
#define SIMULATION_MODE true

#endif // CONFIG_H
