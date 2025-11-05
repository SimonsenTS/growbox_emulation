#include "WebServerManager.h"
#include "WebPage.h"
#include <time.h>

WebServerManager::WebServerManager(SensorManager* sensorManager, DeviceController* deviceController)
    : server(80), sensors(sensorManager), devices(deviceController) {
}

void WebServerManager::begin() {
    // Set up server routes
    server.on("/", [this]() { handleRoot(); });
    server.on(UriBraces("/toggle/{}"), [this]() { handleToggle(); });
    server.on(UriBraces("/brightness/{}"), [this]() { handleBrightness(); });
    server.on("/favicon.ico", [this]() { handleFavicon(); });
    server.onNotFound([this]() { handleNotFound(); });
    
    server.begin();
    Serial.println("HTTP server started (http://localhost:8180)");
}

void WebServerManager::handleRoot() {
    String response = WebPage::getHTML();
    
    float temperature = sensors->readTemperature();
    float humidity = sensors->readHumidity();
    int soilPercentage = sensors->getSoilPercentage();
    int waterPercentage = sensors->getWaterPercentage();

    // Auto-stop pump if soil is too wet
    if (soilPercentage > 80) {
        devices->setPumpState(false);
    }

    if (isnan(temperature) || isnan(humidity)) {
        response.replace("TEMP", "0");
        response.replace("HUM", "0");
    } else {
        int tempPercentage = constrain(temperature, 0, 100);
        int humPercentage = constrain(humidity, 0, 100);
        response.replace("TEMP", String(tempPercentage));
        response.replace("HUM", String(humPercentage));
        response.replace("SOIL", String(soilPercentage));
        response.replace("WATER", String(waterPercentage));

        // Update RGB color based on soil percentage
        devices->updateSoilMoistureColor(soilPercentage);
    }

    // Update device states in the HTML response
    response.replace("PUMP_TEXT", devices->getPumpState() ? "ON" : "OFF");
    response.replace("GrowLED", devices->getGrowLedState() ? "ON" : "OFF");

    // Replace RGB values in HTML
    response.replace("R_VAL", String(devices->getRedValue()));
    response.replace("G_VAL", String(devices->getGreenValue()));
    response.replace("B_VAL", String(devices->getBlueValue()));

    // Replace BRIGHTNESS with the actual brightness value
    response.replace("BRIGHTNESS", String(devices->getBrightness()));

    server.send(200, "text/html", response);
}

void WebServerManager::handleToggle() {
    String button = server.pathArg(0);
    Serial.print("Toggle Button: ");
    Serial.println(button);

    int buttonNumber = button.toInt();
    switch (buttonNumber) {
        case 1:
            devices->togglePump();
            break;
        case 2:
            devices->toggleGrowLed();
            break;
    }

    server.sendHeader("Location", "/");
    server.send(303);
}

void WebServerManager::handleBrightness() {
    String brightnessValue = server.pathArg(0);
    int brightness = brightnessValue.toInt();
    devices->updateGrowLEDBrightness(brightness);
    server.send(204);
}

void WebServerManager::handleNotFound() {
    Serial.print("Unhandled request for: ");
    Serial.println(server.uri());
    server.send(404, "text/plain", "404: Not Found");
}

void WebServerManager::handleFavicon() {
    server.send(204);
}

void WebServerManager::handleClient() {
    server.handleClient();
}

void WebServerManager::initWiFi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void WebServerManager::initTime() {
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
    Serial.println("Time synchronized using NTP");

    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        Serial.println(&timeinfo, "Current time: %Y-%m-%d %H:%M:%S");
    } else {
        Serial.println("Failed to obtain time");
    }
}
