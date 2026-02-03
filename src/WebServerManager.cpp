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
#if SIMULATION_MODE
    server.on("/simulation", HTTP_POST, [this]() { handleSimulation(); });
#endif
    server.on("/favicon.ico", [this]() { handleFavicon(); });
    
    // Captive portal detection routes (for various devices)
    server.on("/generate_204", [this]() { server.sendHeader("Location", "/", true); server.send(302, "text/plain", ""); });
    server.on("/204", [this]() { server.sendHeader("Location", "/", true); server.send(302, "text/plain", ""); });
    server.on("/gen_204", [this]() { server.sendHeader("Location", "/", true); server.send(302, "text/plain", ""); });
    server.on("/hotspot-detect.html", [this]() { server.sendHeader("Location", "/", true); server.send(302, "text/plain", ""); });
    server.on("/canonical.html", [this]() { server.sendHeader("Location", "/", true); server.send(302, "text/plain", ""); });
    server.on("/success.txt", [this]() { server.sendHeader("Location", "/", true); server.send(302, "text/plain", ""); });
    server.on("/ipv6check", [this]() { server.sendHeader("Location", "/", true); server.send(302, "text/plain", ""); });
    
    server.onNotFound([this]() { handleNotFound(); });
    
    server.begin();
    
    // Start DNS server for captive portal (redirect all DNS requests to ESP32)
    dnsServer.start(53, "*", WiFi.softAPIP());
    
    Serial.println("HTTP server started");
    Serial.println("DNS server started (Captive Portal)");
    Serial.println("Access Point: GrowBox_Setup (Open Network)");
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
    // Read water first to avoid interference from soil sensor
    int waterPercentage = sensors->getWaterPercentage();
    int soilPercentage = sensors->getSoilPercentage();

    // Auto-stop pump if water runs out (safety first!)
    if (waterPercentage <= 10 && devices->getPumpState()) {
        devices->setPumpState(false);
    }
    
    // Auto-start pump if soil is too dry (< 20%) and there's water available
    if (soilPercentage < 20 && waterPercentage > 10) {
        devices->setPumpState(true);
    }
    
    // Auto-stop pump if soil is too wet (> 80%)
    if (soilPercentage > 80) {
        devices->setPumpState(false);
    }

    // Update RGB colors based on sensor values (always do this)
    devices->updateSoilMoistureColor(soilPercentage);
    devices->updateWaterLevelColor(waterPercentage);
    
    Serial.println("=== Dashboard RGB Values ===");
    Serial.printf("Soil RGB: R=%d, G=%d, B=%d\n", 
                  devices->getSoilRedValue(), devices->getSoilGreenValue(), devices->getSoilBlueValue());
    Serial.printf("Water RGB: R=%d, G=%d, B=%d\n", 
                  devices->getWaterRedValue(), devices->getWaterGreenValue(), devices->getWaterBlueValue());

    // Replace Soil Moisture RGB values FIRST (before replacing SOIL with percentage)
    String soilR = String(devices->getSoilRedValue());
    String soilG = String(devices->getSoilGreenValue());
    String soilB = String(devices->getSoilBlueValue());
    response.replace("SOIL_R_VAL", soilR);
    response.replace("SOIL_G_VAL", soilG);
    response.replace("SOIL_B_VAL", soilB);
    Serial.printf("Replacing SOIL RGB: %s, %s, %s\n", soilR.c_str(), soilG.c_str(), soilB.c_str());
    
    // Replace Water Level RGB values FIRST (before replacing WATER with percentage)
    String waterR = String(devices->getWaterRedValue());
    String waterG = String(devices->getWaterGreenValue());
    String waterB = String(devices->getWaterBlueValue());
    response.replace("WATER_R_VAL", waterR);
    response.replace("WATER_G_VAL", waterG);
    response.replace("WATER_B_VAL", waterB);
    Serial.printf("Replacing WATER RGB: %s, %s, %s\n", waterR.c_str(), waterG.c_str(), waterB.c_str());

    if (isnan(temperature) || isnan(humidity)) {
        response.replace("TEMP", "0");
        response.replace("HUM", "0");
    } else {
        int tempPercentage = constrain(temperature, 0, 100);
        int humPercentage = constrain(humidity, 0, 100);
        response.replace("TEMP", String(tempPercentage));
        response.replace("HUM", String(humPercentage));
    }
    
    // Replace sensor percentages (do this AFTER RGB replacements)
    response.replace("SOIL", String(soilPercentage));
    response.replace("WATER", String(waterPercentage));

    // Update device states in the HTML response
    response.replace("PUMP_TEXT", devices->getPumpState() ? "ON" : "OFF");
    response.replace("PUMP_CLASS", devices->getPumpState() ? "btn" : "btn-off");
    response.replace("GrowLED", devices->getGrowLedState() ? "ON" : "OFF");
    response.replace("LED_CLASS", devices->getGrowLedState() ? "btn" : "btn-off");
    response.replace("RGB_LED", devices->getRGBLedsEnabled() ? "ON" : "OFF");
    response.replace("RGB_CLASS", devices->getRGBLedsEnabled() ? "btn" : "btn-off");
    response.replace("BOOST_TEXT", devices->getGrowLedBoostState() ? "ON" : "OFF");
    response.replace("BOOST_CLASS", devices->getGrowLedBoostState() ? "btn" : "btn-off");

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
        case 3:
            devices->toggleRGBLeds();
            break;
        case 4:
            devices->toggleGrowLedBoost();
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
    // Silently redirect unhandled captive portal requests
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
}

#if SIMULATION_MODE
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
        // Update RGB LED color immediately
        devices->updateSoilMoistureColor(soil);
    }
    
    if (server.hasArg("water")) {
        int water = server.arg("water").toInt();
        sensors->setSimulatedWaterPercentage(water);
        Serial.print("Simulated Water Level set to: ");
        Serial.println(water);
        // Update RGB LED color immediately
        devices->updateWaterLevelColor(water);
    }
    
    server.send(200, "text/plain", "Simulation values updated");
}
#endif

void WebServerManager::handleFavicon() {
    server.send(204);
}

void WebServerManager::handleClient() {
    dnsServer.processNextRequest();
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
