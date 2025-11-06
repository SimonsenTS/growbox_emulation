#include "WebServerManager.h"
#include "WebPage.h"
#include <time.h>

WebServerManager::WebServerManager(SensorManager* sensorManager, DeviceController* deviceController, AuthManager* authManager)
    : server(80), sensors(sensorManager), devices(deviceController), auth(authManager) {
}

bool WebServerManager::checkAuthentication() {
    if (!auth->isUserAuthenticated()) {
        server.sendHeader("Location", "/");
        server.send(303);
        return false;
    }
    return true;
}

void WebServerManager::begin() {
    // Set up server routes
    server.on("/", [this]() { handleRoot(); });
    server.on("/connect", HTTP_POST, [this]() { handleConnect(); });
    server.on("/scan-networks", HTTP_GET, [this]() { handleScanNetworks(); });
    server.on("/dashboard", HTTP_GET, [this]() { handleDashboard(); });
    server.on(UriBraces("/toggle/{}"), [this]() { handleToggle(); });
    server.on(UriBraces("/brightness/{}"), [this]() { handleBrightness(); });
    server.on("/simulation", HTTP_POST, [this]() { handleSimulation(); });
    server.on("/favicon.ico", [this]() { handleFavicon(); });
    server.onNotFound([this]() { handleNotFound(); });
    
    server.begin();
    Serial.println("HTTP server started");
    Serial.println("Access Point: GrowBox-Setup");
    Serial.println("Password: growbox123");
    Serial.print("URL: http://");
    Serial.println(WiFi.softAPIP());
}

void WebServerManager::handleRoot() {
    // Show login page if not authenticated
    if (!auth->isUserAuthenticated()) {
        String loginPage = auth->getLoginPageHTML();
        server.send(200, "text/html", loginPage);
        return;
    }
    
    // If authenticated, redirect to dashboard
    server.sendHeader("Location", "/dashboard");
    server.send(303);
}

void WebServerManager::handleConnect() {
    String network = server.arg("network");
    String wifi_password = server.arg("wifi_password");
    String username = server.arg("username");
    String password = server.arg("password");

    // First validate the credentials
    if (!auth->validateCredentials(username.c_str(), password.c_str())) {
        server.send(401, "text/html", 
            "<html><body style='font-family: Arial; text-align: center; margin-top: 50px;'>"
            "<h1>Invalid Credentials</h1>"
            "<p>The provided username or password is incorrect</p>"
            "<p><a href='/'>Back to Login</a></p>"
            "</body></html>");
        return;
    }

    if (network.length() > 0 && wifi_password.length() > 0) {
        bool connected = AuthManager::connectToWiFi(network, wifi_password);
        
        if (connected) {
            auth->setAuthenticated(true);
            
            // Restart server to listen on new IP
            server.close();
            delay(1000);
            server.begin();
            
            // Initialize time after WiFi connection
            initTime();
            
            server.send(200, "text/html", 
                "<html><body style='font-family: Arial; text-align: center; margin-top: 50px;'>"
                "<h1>Connected Successfully!</h1>"
                "<p>Network: " + network + "</p>"
                "<p>IP Address: " + WiFi.localIP().toString() + "</p>"
                "<p><a href='/dashboard' style='display: inline-block; margin-top: 20px; padding: 15px 30px; background-color: #4CAF50; color: white; text-decoration: none; border-radius: 5px;'>Go to GrowBox Dashboard</a></p>"
                "</body></html>");
        } else {
            server.send(400, "text/html", 
                "<html><body style='font-family: Arial; text-align: center; margin-top: 50px;'>"
                "<h1>Connection Failed</h1>"
                "<p>Could not connect to " + network + "</p>"
                "<p><a href='/'>Back to Setup</a></p>"
                "</body></html>");
        }
    } else {
        server.send(400, "text/html", 
            "<html><body style='font-family: Arial; text-align: center; margin-top: 50px;'>"
            "<h1>Invalid Parameters</h1>"
            "<p>Network name and password are required</p>"
            "<p><a href='/'>Back to Setup</a></p>"
            "</body></html>");
    }
}

void WebServerManager::handleScanNetworks() {
    auth->scanNetworks();
    server.send(200, "text/plain", "Networks scanned");
}

void WebServerManager::handleDashboard() {
    if (!checkAuthentication()) {
        return;
    }
    
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
    if (!checkAuthentication()) {
        return;
    }
    
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

    server.sendHeader("Location", "/dashboard");
    server.send(303);
}

void WebServerManager::handleBrightness() {
    if (!checkAuthentication()) {
        return;
    }
    
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

void WebServerManager::handleSimulation() {
    if (!checkAuthentication()) {
        return;
    }
    
    if (server.hasArg("temperature")) {
        float temp = server.arg("temperature").toFloat();
        sensors->setSimulatedTemperature(temp);
        Serial.print("Simulated Temperature set to: ");
        Serial.println(temp);
    }
    
    if (server.hasArg("humidity")) {
        float hum = server.arg("humidity").toFloat();
        sensors->setSimulatedHumidity(hum);
        Serial.print("Simulated Humidity set to: ");
        Serial.println(hum);
    }
    
    if (server.hasArg("soil")) {
        int soil = server.arg("soil").toInt();
        sensors->setSimulatedSoilPercentage(soil);
        Serial.print("Simulated Soil Moisture set to: ");
        Serial.println(soil);
    }
    
    if (server.hasArg("water")) {
        int water = server.arg("water").toInt();
        sensors->setSimulatedWaterPercentage(water);
        Serial.print("Simulated Water Level set to: ");
        Serial.println(water);
    }
    
    server.send(200, "text/plain", "Simulation values updated");
}

void WebServerManager::handleFavicon() {
    server.send(204);
}

void WebServerManager::handleClient() {
    server.handleClient();
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
