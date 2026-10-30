#include <Arduino.h>
#include "HardwareRuntime.h"
#include "AuthManager.h"
#include "WebServerManager.h"

HardwareRuntime hardware;
AuthManager auth("admin", "password123");
WebServerManager webServer(&hardware, &auth);

void setup() {
    Serial.begin(115200);
    if (!hardware.begin()) {
        Serial.println("FATAL: RTOS allocation failed; pump remains OFF. Reset required.");
        for (;;) delay(1000);
    }
    AuthManager::initAccessPoint();
    auth.begin();
    auth.scanNetworks();
    webServer.begin();
    Serial.println("GrowBox ready: connect to GrowBox_Setup and open http://192.168.4.1");
}
void loop() {
    webServer.handleClient(); // DNS, WiFi progress and change-only SSE publication
    delay(2);
}
