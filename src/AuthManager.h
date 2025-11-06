#ifndef AUTHMANAGER_H
#define AUTHMANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <vector>

class AuthManager {
private:
    const char* username;
    const char* password;
    bool isAuthenticated;
    std::vector<String> networkList;
    
public:
    AuthManager(const char* user = "admin", const char* pass = "password123");
    
    bool validateCredentials(const char* user, const char* pass) const;
    bool isUserAuthenticated() const { return isAuthenticated; }
    void setAuthenticated(bool state) { isAuthenticated = state; }
    
    void scanNetworks();
    String getNetworksAsOptions() const;
    String getLoginPageHTML() const;
    
    static void initAccessPoint();
    static bool connectToWiFi(const String& ssid, const String& password);
};

#endif // AUTHMANAGER_H
