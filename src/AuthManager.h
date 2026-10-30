#ifndef AUTHMANAGER_H
#define AUTHMANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include <atomic>
#include <mutex>

class AuthManager {
private:
    const char* username;
    const char* password;
    uint8_t persistentKey[32]{};
    uint8_t sessionKey[32]{};
    bool persistentReady = false;
    String signature(const String& payload, bool remember) const;
    mutable std::mutex networkMutex;
    std::atomic<bool> scanning{false};
    std::vector<String> networkList;
    
public:
    AuthManager(const char* user = "admin", const char* pass = "password123");
    
    bool validateCredentials(const char* user, const char* pass) const;
    void begin();
    String createSession(bool remember) const;
    bool isUserAuthenticated(const String& cookieHeader) const;
    
    void scanNetworks();
    void pollNetworks();
    bool isScanning() const { return scanning.load(); }
    String getNetworksAsOptions() const;
    String getLoginPageHTML() const;
    
    static void initAccessPoint();
};

#endif // AUTHMANAGER_H
