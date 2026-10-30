#pragma once
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include "HardwareRuntime.h"
#include "AuthManager.h"

class WebServerManager {
public:
    WebServerManager(HardwareRuntime* runtime, AuthManager* authManager);
    void begin();
    void handleClient();
private:
    struct WiFiRequest { char ssid[33]; char password[65]; };
    AsyncWebServer server{80};
    AsyncEventSource events{"/events"};
    DNSServer dnsServer;
    HardwareRuntime* hardware;
    AuthManager* auth;
    QueueHandle_t wifiRequests = nullptr;
    std::atomic<bool> connecting{false};
    uint32_t connectStarted = 0;
    uint32_t lastRevision = UINT32_MAX;
    uint32_t lastHeartbeat = 0;
    uint32_t lastNetworkCheck = 0;
    String lastNetwork;
    bool checkAuthentication(AsyncWebServerRequest* request);
    void handleConnect(AsyncWebServerRequest* request);
    void handleControl(AsyncWebServerRequest* request);
    void handleSimulation(AsyncWebServerRequest* request);
    void handleWiFi();
    String stateJSON(const GrowboxState& state);
    String networkJSON();
};
