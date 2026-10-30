#ifndef CONFIG_H
#define CONFIG_H

// WiFi configuration is now handled through the web interface
// No need for hardcoded credentials - users configure via GrowBox_Setup AP
#define GROWBOX_HOSTNAME "growbox-dashboard"
#define WIFI_CONNECTION_TIMEOUT_MS 20000
#define WIFI_RETRY_INTERVAL_MS 30000

// SHT40 sensor configuration (I2C)
// ESP32-S3-Mini-1: Only GPIO 0-21 are exposed. GPIO 22+ are used internally for flash.
// Avoid strapping pins: 0, 3, 45, 46. Avoid USB: 19, 20. Avoid boot issues: 8, 9.
#define SHT40_SDA 18
#define SHT40_SCL 17

// LTR-303ALS-01 ambient light sensor — to be added in PCB v2 with correct SDA/SCL orientation
// #define LTR303_INT_PIN 21

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
#define WATER_LEVEL_THRESHOLD 50        // Capacitive touch threshold (raw values are 0=dry, ~252=wet)
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
#define SOIL_LED_PIN 35        // Data pin for soil moisture WS2812B
#define WATER_LED_PIN 36       // Data pin for water level WS2812B
#define NUM_LEDS 1             // One LED per strip

// Time configuration
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 0
#define DAYLIGHT_OFFSET_SEC 3600

// Button debounce
#define DEBOUNCE_DELAY 200

// Sensors run independently of HTTP. Must stay > 0 for pump protection.
#define AUTO_SENSOR_INTERVAL 1000
#define SENSOR_STALE_MS 4000
#define CONTROL_INTERVAL_MS 20

// Optional PCB v2 LTR-303 on the shared I2C bus (0x29).
#ifndef ENABLE_LUX_SENSOR
#define ENABLE_LUX_SENSOR 0
#endif

// Simulation mode - set to true to enable manual sensor input
#ifndef SIMULATION_MODE
#define SIMULATION_MODE 0
#endif

#endif // CONFIG_H
