#ifndef CONFIG_H
#define CONFIG_H

// WiFi configuration
#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
//#define WIFI_SSID "Thomas"
//#define WIFI_PASSWORD "Kdawg123"
//#define WIFI_SSID "ARRIS-1925-5G"
//#define WIFI_PASSWORD "FmBNaNuERgJB"
#define WIFI_CHANNEL 6

// DHT sensor configuration
#define DHTPIN 4
#define DHTTYPE DHT22

// Sensor pins
#define SOIL_SENSOR_PIN 33
#define WATER_SENSOR_PIN 34
#define BUTTON_PIN 32

// PWM configuration
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8
#define RED_CHANNEL 0
#define GREEN_CHANNEL 1
#define BLUE_CHANNEL 2

// Relay and LED pins
#define PUMP_RELAY 26
#define GROWLED_RELAY 27
#define GROWLED_PWM 5

// RGB LED pins
#define RED_PIN 23
#define GREEN_PIN 22
#define BLUE_PIN 21

// Time configuration
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 0
#define DAYLIGHT_OFFSET_SEC 3600

// Button debounce
#define DEBOUNCE_DELAY 200

#endif // CONFIG_H
