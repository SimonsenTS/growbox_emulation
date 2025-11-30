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
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);
    
    // Create open AP with explicit parameters: SSID, no password, channel 1, hidden=false, max connections=4
    bool apSuccess = WiFi.softAP("GrowBox_Setup", NULL, 1, false, 4);
    
    if (apSuccess) {
        IPAddress IP = WiFi.softAPIP();
        Serial.println("Access Point Started Successfully");
        Serial.print("AP IP address: ");
        Serial.println(IP);
        Serial.println("Connect to WiFi: GrowBox_Setup (OPEN - NO PASSWORD)");
        Serial.println("Go to: http://192.168.4.1");
    } else {
        Serial.println("Access Point Failed to Start");
    }
    
    delay(500);
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
