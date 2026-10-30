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
    WiFiRequest savedWiFi{};
    WiFiRequest activeWiFi{};
    std::atomic<bool> haveSavedWiFi{false};
    std::atomic<bool> wifiSaveFailed{false};
    std::atomic<bool> wifiAttemptFailed{false};
    std::atomic<bool> mdnsReady{false};
    bool saveActiveWiFi = false;
    bool wasConnected = false;
    std::atomic<bool> connecting{false};
    std::atomic<bool> scanRequested{false};
    uint32_t connectStarted = 0;
    uint32_t lastMDNSAttempt = 0;
    uint32_t lastRevision = UINT32_MAX;
    uint32_t lastHeartbeat = 0;
    uint32_t lastNetworkCheck = 0;
    String lastNetwork;
    bool checkAuthentication(AsyncWebServerRequest* request);
    void handleConnect(AsyncWebServerRequest* request);
    void handleControl(AsyncWebServerRequest* request);
    void handleSimulation(AsyncWebServerRequest* request);
    void handleWiFi();
    void loadSavedWiFi();
    void startWiFi(const WiFiRequest& credentials, bool saveOnSuccess);
    void startMDNS();
    String stateJSON(const GrowboxState& state);
    String networkJSON();
};
