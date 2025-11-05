#ifndef WEBSERVERMANAGER_H
#define WEBSERVERMANAGER_H

#include <WiFi.h>
#include <WebServer.h>
#include <uri/UriBraces.h>
#include "Config.h"
#include "SensorManager.h"
#include "DeviceController.h"

class WebServerManager {
private:
    WebServer server;
    SensorManager* sensors;
    DeviceController* devices;
    
    void handleRoot();
    void handleToggle();
    void handleBrightness();
    void handleNotFound();
    void handleFavicon();
    
public:
    WebServerManager(SensorManager* sensorManager, DeviceController* deviceController);
    void begin();
    void handleClient();
    
    static void initWiFi();
    static void initTime();
};

#endif // WEBSERVERMANAGER_H
