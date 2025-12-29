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
    // Read water first to avoid interference from soil sensor
    int initialWater = sensors.getWaterPercentage();
    int initialSoil = sensors.getSoilPercentage();
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
    Serial.println("1. Connect to WiFi: GrowBox_Setup (Open Network)");
    Serial.println("2. Open browser: http://192.168.4.1");
    Serial.println("3. Configure WiFi and set password");
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
        // Read water first to avoid interference from soil sensor
        int waterPercentage = sensors.getWaterPercentage();
        int soilPercentage = sensors.getSoilPercentage();
        
        // Update RGB LED colors
        devices.updateSoilMoistureColor(soilPercentage);
        devices.updateWaterLevelColor(waterPercentage);
        
        // Log readings first
        Serial.println("=== Automatic Sensor Reading ===");
        Serial.printf("Temperature: %.1f°C, Humidity: %.1f%%\n", temperature, humidity);
        Serial.printf("Soil: %d%%, Water: %d%%\n", soilPercentage, waterPercentage);
        Serial.printf("Current Pump State: %s\n", devices.getPumpState() ? "ON" : "OFF");
        
        // Pump control logic (priority order)
        
        // 1. SAFETY: Auto-stop pump if water runs out (highest priority!)
        if (waterPercentage <= 10 && devices.getPumpState()) {
            devices.setPumpState(false);
            Serial.println(">>> AUTO-STOP: Water level too low (<= 10%) - PUMP PROTECTION");
        }
        // 2. Auto-stop pump if soil is too wet (> 80%)
        else if (soilPercentage > 80 && devices.getPumpState()) {
            devices.setPumpState(false);
            Serial.println(">>> AUTO-STOP: Soil moisture > 80% (WET - GREEN LED)");
        }
        // 3. Auto-start pump if soil is too dry (< 20%) and water is available
        else if (soilPercentage < 20 && waterPercentage > 10 && !devices.getPumpState()) {
            devices.setPumpState(true);
            Serial.println(">>> AUTO-START: Soil moisture < 20% (DRY - RED LED)");
        }
    }
#endif

    // Handle client requests
    webServer.handleClient();
    delay(2);
}
