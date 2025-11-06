#ifndef WEBSERVERMANAGER_H
#define WEBSERVERMANAGER_H

#include <WiFi.h>
#include <WebServer.h>
#include <uri/UriBraces.h>
#include "Config.h"
#include "SensorManager.h"
#include "DeviceController.h"
#include "AuthManager.h"

class WebServerManager {
private:
    WebServer server;
    SensorManager* sensors;
    DeviceController* devices;
    AuthManager* auth;
    
    bool checkAuthentication();
    void handleRoot();
    void handleLogin();
    void handleConnect();
    void handleScanNetworks();
    void handleDashboard();
    void handleToggle();
    void handleBrightness();
    void handleSimulation();
    void handleNotFound();
    void handleFavicon();
    
public:
    WebServerManager(SensorManager* sensorManager, DeviceController* deviceController, AuthManager* authManager);
    void begin();
    void handleClient();
    
    static void initTime();
};

#endif // WEBSERVERMANAGER_H
