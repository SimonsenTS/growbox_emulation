#include <Arduino.h>
#include <WiFi.h>
#include "Config.h"
#include "SensorManager.h"
#include "DeviceController.h"
#include "AuthManager.h"
#include "WebServerManager.h"

// Create instances of our managers
SensorManager sensors;
DeviceController devices;
AuthManager auth("admin", "password123");  // Default credentials
WebServerManager webServer(&sensors, &devices, &auth);

// Automatic sensor reading timing
unsigned long lastSensorReadTime = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n=================================");
    Serial.println("GrowBox System Starting...");
    Serial.println("=================================\n");
    
    // Initialize all components
    sensors.begin();
    devices.begin();
    
    // Initialize RGB LED colors based on initial sensor readings
    int initialSoil = sensors.getSoilPercentage();
    int initialWater = sensors.getWaterPercentage();
    devices.updateSoilMoistureColor(initialSoil);
    devices.updateWaterLevelColor(initialWater);
    Serial.printf("Initial RGB colors set - Soil: %d%%, Water: %d%%\n", initialSoil, initialWater);
    
    // Initialize Access Point for initial setup
    AuthManager::initAccessPoint();
    
    // Scan for available networks
    auth.scanNetworks();
    
    // Start web server
    webServer.begin();
    
    Serial.println("\n=================================");
    Serial.println("System Ready!");
    Serial.println("1. Connect to WiFi: GrowBox-Setup");
    Serial.println("2. Password: growbox123");
    Serial.println("3. Open browser: http://192.168.4.1");
    Serial.println("4. Login: admin / password123");
    Serial.println("=================================\n");
}

void loop() {
    // Check and handle button press
    if (devices.checkButton()) {
        devices.togglePump();
        Serial.print("Pump State: ");
        Serial.println(devices.getPumpState() ? "ON" : "OFF");
    }

#if AUTO_SENSOR_INTERVAL > 0
    // Periodic automatic sensor reading
    unsigned long currentTime = millis();
    if (currentTime - lastSensorReadTime >= AUTO_SENSOR_INTERVAL) {
        lastSensorReadTime = currentTime;
        
        // Read all sensors
        float temperature = sensors.readTemperature();
        float humidity = sensors.readHumidity();
        int soilPercentage = sensors.getSoilPercentage();
        int waterPercentage = sensors.getWaterPercentage();
        
        // Update RGB LED colors
        devices.updateSoilMoistureColor(soilPercentage);
        devices.updateWaterLevelColor(waterPercentage);
        
        // Auto-stop pump if soil is too wet
        if (soilPercentage > 80) {
            devices.setPumpState(false);
            Serial.println("Auto-stop: Soil moisture > 80%");
        }
        
        // Log readings
        Serial.println("=== Automatic Sensor Reading ===");
        Serial.printf("Temperature: %.1f°C, Humidity: %.1f%%\n", temperature, humidity);
        Serial.printf("Soil: %d%%, Water: %d%%\n", soilPercentage, waterPercentage);
    }
#endif

    // Handle client requests
    webServer.handleClient();
    delay(2);
}
