#ifndef CONFIG_H
#define CONFIG_H

// WiFi configuration is now handled through the web interface
// No need for hardcoded credentials - users configure via GrowBox_Setup AP

// SHT40 sensor configuration (I2C)
// SDA = GPIO 21, SCL = GPIO 22 (default ESP32 I2C pins)
#define SHT40_SDA 21
#define SHT40_SCL 22

// Sensor pins
#define SOIL_SENSOR_PIN 33
#define WATER_SENSOR_PIN 34
#define BUTTON_PIN 32

// Sensor calibration values (adjust based on your 3.3V sensors)
// For soil sensor: dry = high value, wet = low value (inverted)
#define SOIL_DRY_VALUE 4095    // Analog reading when completely dry
#define SOIL_WET_VALUE 549    // Analog reading when fully submerged
// For water sensor: empty = low value, full = high value (normal)
#define WATER_EMPTY_VALUE 0    // Analog reading when empty/dry (no power when dry)
#define WATER_FULL_VALUE 1460  // Analog reading when fully submerged

// Sensor power control pins (to prevent corrosion)
#define SOIL_POWER_PIN 25
#define WATER_POWER_PIN 14

// PWM configuration
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8

// Relay and LED pins
#define PUMP_RELAY 26
#define GROWLED_RELAY 27
#define GROWLED_PWM 5

// WS2812B RGB LED pins (addressable)
#define SOIL_LED_PIN 23      // Data pin for soil moisture WS2812B
#define WATER_LED_PIN 19     // Data pin for water level WS2812B
#define NUM_LEDS 1           // One LED per strip

// Time configuration
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 0
#define DAYLIGHT_OFFSET_SEC 3600

// Button debounce
#define DEBOUNCE_DELAY 200

// Automatic sensor reading interval (in milliseconds)
// Set to 0 to disable periodic readings (only read on dashboard access)
// Default: 10000 ms = 10 seconds (for responsive auto pump control)
#define AUTO_SENSOR_INTERVAL 10000

// Simulation mode - set to true to enable manual sensor input
#define SIMULATION_MODE false

#endif // CONFIG_H
