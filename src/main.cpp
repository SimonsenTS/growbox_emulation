#include <Arduino.h>
#include <WiFi.h>
#include "Config.h"
#include "SensorManager.h"
#include "DeviceController.h"
#include "WebServerManager.h"

// Create instances of our managers
SensorManager sensors;
DeviceController devices;
WebServerManager webServer(&sensors, &devices);

void setup() {
    Serial.begin(115200);
    
    // Initialize all components
    sensors.begin();
    devices.begin();
    
    // Connect to WiFi
    WebServerManager::initWiFi();
    
    // Initialize time
    WebServerManager::initTime();
    
    // Start web server
    webServer.begin();
}

void loop() {
    // Reconnect WiFi if needed
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.reconnect();
    }

    // Check and handle button press
    if (devices.checkButton()) {
        devices.togglePump();
        Serial.print("Pump State: ");
        Serial.println(devices.getPumpState() ? "ON" : "OFF");
    }

    // Handle client requests
    webServer.handleClient();
    delay(2);
}
