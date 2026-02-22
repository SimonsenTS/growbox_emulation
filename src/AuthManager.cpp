#include "AuthManager.h"
#include <cstring>

AuthManager::AuthManager(const char* user, const char* pass)
    : username(user), password(pass), isAuthenticated(false) {
}

bool AuthManager::validateCredentials(const char* user, const char* pass) const {
    if (!user || !pass) {
        return false;
    }
    return (strcmp(user, username) == 0 && strcmp(pass, password) == 0);
}

void AuthManager::scanNetworks() {
    networkList.clear();
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; ++i) {
        networkList.push_back(WiFi.SSID(i));
    }
}

String AuthManager::getNetworksAsOptions() const {
    String options;
    for (const String& network : networkList) {
        options += "<option value=\"" + network + "\">" + network + "</option>";
    }
    return options;
}

String AuthManager::getLoginPageHTML() const {
    String html = R"delimiter(
<!DOCTYPE html>
<html>
<head>
    <title>GrowBox Login</title>
    <style>
        body { 
            font-family: Arial; 
            text-align: center; 
            margin-top: 50px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
        }
        .login-form { 
            display: inline-block; 
            padding: 30px; 
            background: white;
            border-radius: 10px;
            box-shadow: 0 10px 25px rgba(0,0,0,0.3);
            color: #333;
        }
        input, select { 
            margin: 10px; 
            padding: 12px; 
            width: 250px;
            border: 2px solid #ddd;
            border-radius: 5px;
            font-size: 14px;
        }
        input:focus, select:focus {
            outline: none;
            border-color: #667eea;
        }
        .title { 
            color: white; 
            margin-bottom: 30px;
            font-size: 2.5em;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
        }
        .subtitle {
            color: #333;
            margin-bottom: 20px;
            font-size: 1.5em;
        }
        .refresh-btn, .submit-btn {
            background-color: #667eea;
            color: white;
            border: none;
            padding: 12px 20px;
            cursor: pointer;
            border-radius: 5px;
            font-size: 14px;
            margin: 5px;
            transition: background-color 0.3s;
        }
        .refresh-btn:hover, .submit-btn:hover {
            background-color: #764ba2;
        }
        .submit-btn {
            background-color: #4CAF50;
            padding: 12px 40px;
            font-size: 16px;
            font-weight: bold;
        }
        .submit-btn:hover {
            background-color: #45a049;
        }
    </style>
    <script>
        function refreshNetworks() {
            fetch('/scan-networks')
                .then(response => window.location.reload());
        }
    </script>
</head>
<body>
    <h1 class="title">GrowBox System</h1>
    <div class="login-form">
        <h2 class="subtitle">WiFi Setup & Login</h2>
        <form action="/connect" method="POST">
            <select name="network" required>
                <option value="">Select WiFi Network</option>
                %NETWORK_LIST%
            </select><br>
            <input type="password" name="wifi_password" placeholder="WiFi Password" required><br>
            <input type="text" name="username" placeholder="Login Username" required><br>
            <input type="password" name="password" placeholder="Login Password" required><br>
            <button type="button" class="refresh-btn" onclick="refreshNetworks()">Refresh Networks</button><br>
            <input type="submit" class="submit-btn" value="Connect & Login">
        </form>
    </div>
</body>
</html>
)delimiter";
    
    html.replace("%NETWORK_LIST%", getNetworksAsOptions());
    return html;
}

void AuthManager::initAccessPoint() {
    Serial.println("Initializing WiFi Access Point for ESP32-S3...");
    
    // ESP32-S3 specific: Complete reset and initialization sequence
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(500);
    
    // Enable WiFi in AP mode
    WiFi.mode(WIFI_AP);
    delay(500);
    
    // Configure and start AP with simpler parameters for ESP32-S3
    Serial.println("Starting Access Point: GrowBox_Setup");
    bool apSuccess = WiFi.softAP("GrowBox_Setup");
    
    delay(1000);  // Give more time for AP to stabilize on S3
    
    if (apSuccess) {
        IPAddress IP = WiFi.softAPIP();
        Serial.println("========================================");
        Serial.println("Access Point Started Successfully!");
        Serial.print("AP SSID: GrowBox_Setup");
        Serial.println(" (OPEN - NO PASSWORD)");
        Serial.print("AP IP address: ");
        Serial.println(IP);
        Serial.println("Connect to: http://192.168.4.1");
        Serial.println("========================================");
    } else {
        Serial.println("!!! Access Point Failed to Start !!!");
        Serial.println("Attempting alternative AP configuration...");
        
        // Alternative approach with explicit IP configuration
        WiFi.mode(WIFI_OFF);
        delay(500);
        WiFi.mode(WIFI_AP);
        delay(500);
        
        IPAddress local_IP(192, 168, 4, 1);
        IPAddress gateway(192, 168, 4, 1);
        IPAddress subnet(255, 255, 255, 0);
        
        WiFi.softAPConfig(local_IP, gateway, subnet);
        apSuccess = WiFi.softAP("GrowBox_Setup", "", 1, 0, 4);
        
        delay(1000);
        
        if (apSuccess) {
            IPAddress IP = WiFi.softAPIP();
            Serial.println("Access Point Started on Retry!");
            Serial.print("AP IP address: ");
            Serial.println(IP);
        } else {
            Serial.println("!!! CRITICAL: AP Failed on Retry !!!");
            Serial.println("Please reset the device");
        }
    }
}

bool AuthManager::connectToWiFi(const String& ssid, const String& password) {
    if (WiFi.status() == WL_CONNECTED && WiFi.SSID() == ssid) {
        Serial.println("Already connected to the requested network!");
        return true;
    }
    
    WiFi.disconnect(true);
    delay(1000);
    WiFi.mode(WIFI_AP_STA);
    delay(1000);
    
    WiFi.begin(ssid.c_str(), password.c_str());
    
    Serial.print("Attempting to connect to: ");
    Serial.println(ssid);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected successfully!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        return true;
    }
    
    Serial.println("\nConnection failed!");
    return false;
}
