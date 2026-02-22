#ifndef CONFIG_H
#define CONFIG_H

// WiFi configuration is now handled through the web interface
// No need for hardcoded credentials - users configure via GrowBox_Setup AP

// SHT40 sensor configuration (I2C)
// ESP32-S3: Must use valid I2C pins (21/22 don't work on S3!)
// Common S3 I2C pins: GPIO 8/9, GPIO 35/36, or GPIO 1/2
#define SHT40_SDA 8
#define SHT40_SCL 9

// Sensor pins (ESP32-S3 compatible - ADC1 channels)
#define SOIL_SENSOR_PIN 4      // ADC1_CH3
#define WATER_SENSOR_PIN 5     // Not used - Grove Water Level uses I2C (ADC1_CH4)
#define BUTTON_PIN 0           // BOOT button or external button

// Sensor calibration values (adjust based on your 3.3V sensors)
// For soil sensor: dry = high value, wet = low value (inverted)
#define SOIL_DRY_VALUE 4095    // Analog reading when completely dry
#define SOIL_WET_VALUE 549    // Analog reading when fully submerged

// Grove Water Level Sensor I2C configuration
// This sensor uses I2C interface with capacitive sensing
// Detects water levels from 0-10cm with ±5mm accuracy
#define WATER_LEVEL_I2C_ADDR_HIGH 0x78  // Upper 12 sections
#define WATER_LEVEL_I2C_ADDR_LOW  0x77  // Lower 8 sections
#define WATER_LEVEL_THRESHOLD 100       // Capacitive touch threshold
#define WATER_LEVEL_MAX_SECTIONS 20     // Total 20 sections (8 low + 12 high)

// Sensor power control pins (to prevent corrosion)
#define SOIL_POWER_PIN 6       // ESP32-S3 safe GPIO
#define WATER_POWER_PIN 7      // Not used for Grove Water Level Sensor (I2C powered)

// PWM configuration
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8

// Relay and LED pins (ESP32-S3 compatible - GPIO 26-32 don't exist)
#define PUMP_RELAY 10          // Safe GPIO for relay
#define GROWLED_RELAY 11       // Safe GPIO for relay
#define GROWLED_PWM 12         // PWM capable pin
#define GROWLED_BOOST 13       // Third wire for additional grow LED power (toggle HIGH/LOW)

// WS2812B RGB LED pins (addressable) - ESP32-S3 compatible
#define SOIL_LED_PIN 1         // Data pin for soil moisture WS2812B
#define WATER_LED_PIN 2        // Data pin for water level WS2812B
#define NUM_LEDS 1             // One LED per strip

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
