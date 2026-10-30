#include "AuthManager.h"
#include <cstring>
#include <Preferences.h>
#include <esp_random.h>
#include <mbedtls/md.h>
#include <time.h>

AuthManager::AuthManager(const char* user, const char* pass)
    : username(user), password(pass) {
}

bool AuthManager::validateCredentials(const char* user, const char* pass) const {
    if (!user || !pass) {
        return false;
    }
    return (strcmp(user, username) == 0 && strcmp(pass, password) == 0);
}

// Persist a random signing key, never a browser's login password.
void AuthManager::begin() {
    esp_fill_random(sessionKey, sizeof(sessionKey));
    Preferences preferences;
    if (preferences.begin("growbox-auth", false)) {
        if (preferences.getBytesLength("key") == sizeof(persistentKey))
            persistentReady = preferences.getBytes("key", persistentKey, sizeof(persistentKey)) == sizeof(persistentKey);
        else {
            esp_fill_random(persistentKey, sizeof(persistentKey));
            persistentReady = preferences.putBytes("key", persistentKey, sizeof(persistentKey)) == sizeof(persistentKey);
        }
        preferences.end();
    }
}
static String hexBytes(const uint8_t* data, size_t length) {
    static const char hex[] = "0123456789abcdef";
    String value;
    value.reserve(length * 2);
    for (size_t i = 0; i < length; ++i) { value += hex[data[i] >> 4]; value += hex[data[i] & 15]; }
    return value;
}
String AuthManager::signature(const String& payload, bool remember) const {
    // Credential changes in firmware invalidate existing tokens.
    const String message = payload + "\n" + username + "\n" + password;
    uint8_t hash[32];
    if (mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
        remember ? persistentKey : sessionKey, 32,
        reinterpret_cast<const unsigned char*>(message.c_str()), message.length(), hash) != 0) return "";
    return hexBytes(hash, sizeof(hash));
}
String AuthManager::createSession(bool remember) const {
    if (remember && !persistentReady) return "";
    uint8_t nonce[16];
    esp_fill_random(nonce, sizeof(nonce));
    const time_t now = time(nullptr);
    // Before NTP, the browser enforces the cookie's 30-day Max-Age.
    const uint32_t expires = now >= 1700000000 ? uint32_t(now + 30 * 24 * 60 * 60) : 0;
    const String payload = String(remember ? "p." : "s.") + hexBytes(nonce, sizeof(nonce)) + "." + String(expires);
    const String mac = signature(payload, remember);
    return mac.isEmpty() ? String("") : payload + "." + mac;
}
bool AuthManager::isUserAuthenticated(const String& cookieHeader) const {
    if (cookieHeader.length() > 2048) return false;
    String token;
    int start = 0;
    while (start < int(cookieHeader.length())) {
        int end = cookieHeader.indexOf(';', start);
        if (end < 0) end = cookieHeader.length();
        String cookie = cookieHeader.substring(start, end);
        cookie.trim();
        if (cookie.startsWith("GrowBoxSession=")) { token = cookie.substring(15); break; }
        start = end + 1;
    }
    if (token.length() < 101 || token.length() > 110 || token[1] != '.' || token[34] != '.') return false;
    const bool remember = token[0] == 'p';
    if ((!remember && token[0] != 's') || (remember && !persistentReady)) return false;
    for (int i = 2; i < 34; ++i) if (!isHexadecimalDigit(token[i])) return false;
    const int split = token.indexOf('.', 35);
    if (split < 36 || split > 45 || token.length() - split - 1 != 64) return false;
    for (int i = 35; i < split; ++i) if (!isDigit(token[i])) return false;
    const unsigned long expires = strtoul(token.substring(35, split).c_str(), nullptr, 10);
    const time_t now = time(nullptr);
    if (expires && now >= 1700000000 && uint64_t(now) >= expires) return false;
    const String expected = signature(token.substring(0, split), remember);
    if (expected.length() != 64) return false;
    uint8_t difference = 0;
    for (int i = 0; i < 64; ++i) difference |= uint8_t(expected[i] ^ token[split + 1 + i]);
    return difference == 0;
}

static String escapeHTML(String value) {
    value.replace("&", "&amp;");
    value.replace("\"", "&quot;");
    value.replace("<", "&lt;");
    value.replace(">", "&gt;");
    return value;
}
void AuthManager::scanNetworks() {
    if (scanning.exchange(true)) return;
    const int result = WiFi.scanNetworks(true);
    if (result == WIFI_SCAN_FAILED) scanning.store(false);
}
void AuthManager::pollNetworks() {
    if (!scanning.load()) return;
    const int count = WiFi.scanComplete();
    if (count == WIFI_SCAN_RUNNING) return;
    if (count >= 0) {
        std::lock_guard<std::mutex> lock(networkMutex);
        networkList.clear();
        for (int i = 0; i < count; ++i) networkList.push_back(WiFi.SSID(i));
        WiFi.scanDelete();
    }
    scanning.store(false);
}

String AuthManager::getNetworksAsOptions() const {
    std::lock_guard<std::mutex> lock(networkMutex);
    String options;
    for (const String& network : networkList) {
        options += "<option value=\"" + escapeHTML(network) + "\">" + escapeHTML(network) + "</option>";
    }
    return options;
}

String AuthManager::getLoginPageHTML() const {
    String html = R"delimiter(
<!DOCTYPE html>
<html>
<head>
    <title>GrowBox Login</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta charset="utf-8">
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
        .remember { display:block; margin:12px; font-size:14px; }
        .remember input { width:auto; margin:0 8px 0 0; padding:0; }
        .submit-btn:hover {
            background-color: #45a049;
        }
    </style>
    <script>
        let networkTimer;
        async function readNetworks() {
            try {
                const response = await fetch('/api/networks', {cache: 'no-store'});
                if (!response.ok) throw new Error('Network list unavailable');
                const state = await response.json();
                if (state.scanning) {
                    networkTimer = setTimeout(readNetworks, 1000);
                    return;
                }
                const select = document.querySelector('[name="network"]');
                const selected = select.value;
                select.innerHTML = '<option value="">Use setup WiFi only</option>' + state.options;
                if ([...select.options].some(option => option.value === selected)) select.value = selected;
            } catch (error) { console.warn(error); }
        }
        async function refreshNetworks() {
            clearTimeout(networkTimer);
            try { await fetch('/scan-networks', {method: 'POST'}); } catch (error) { console.warn(error); }
            readNetworks();
        }
        window.addEventListener('DOMContentLoaded', readNetworks);
    </script>
</head>
<body>
    <h1 class="title">GrowBox System</h1>
    <div class="login-form">
        <h2 class="subtitle">WiFi Setup & Login</h2>
        <form action="/connect" method="POST" autocomplete="on">
            <select name="network">
                <option value="">Use setup WiFi only</option>
                %NETWORK_LIST%
            </select><br>
            <input type="password" name="wifi_password" autocomplete="off" placeholder="WiFi Password (if needed)"><br>
            <input type="text" name="username" id="username" autocomplete="username" autocapitalize="none" spellcheck="false" placeholder="Login Username" required><br>
            <input type="password" name="password" id="password" autocomplete="current-password" placeholder="Login Password" required><br>
            <label class="remember"><input type="checkbox" name="remember" value="1" checked>Keep me signed in for 30 days</label>
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
    WiFi.mode(WIFI_AP_STA);
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
        WiFi.mode(WIFI_AP_STA);
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
