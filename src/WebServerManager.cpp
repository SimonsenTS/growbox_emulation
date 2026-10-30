#include "WebServerManager.h"
#include "WebAssets.h"
#include <math.h>
#include <ESPmDNS.h>
#include <Preferences.h>

namespace {
void json(AsyncWebServerRequest* request, int code, const String& body) {
    auto* response = request->beginResponse(code, "application/json", body);
    response->addHeader("Cache-Control", "no-store");
    request->send(response);
}
String quoteJSON(const String& value) {
    String escaped = "\"";
    for (size_t i = 0; i < value.length(); ++i) {
        const unsigned char c = value[i];
        if (c == '"' || c == '\\') { escaped += '\\'; escaped += char(c); }
        else if (c < 0x20) { char hex[7]; snprintf(hex, sizeof(hex), "\\u%04x", c); escaped += hex; }
        else escaped += char(c);
    }
    return escaped + "\"";
}
String number(float value, bool valid, int decimals = 1) {
    return valid && isfinite(value) ? String(value, decimals) : "null";
}
bool parseInteger(const String& value, int maximum, int& result) {
    if (value.isEmpty() || value.length() > 3) return false;
    for (size_t i = 0; i < value.length(); ++i) if (!isDigit(value[i])) return false;
    result = value.toInt();
    return result <= maximum;
}
void asset(AsyncWebServerRequest* request, const char* type, const uint8_t* data, size_t size) {
    auto* response = request->beginResponse(200, type, data, size);
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "no-cache");
    request->send(response);
}
}

WebServerManager::WebServerManager(HardwareRuntime* runtime, AuthManager* authManager)
    : hardware(runtime), auth(authManager) {}

bool WebServerManager::checkAuthentication(AsyncWebServerRequest* request) {
    if (auth->isUserAuthenticated(request->header("Cookie"))) return true;
    json(request, 401, "{\"error\":\"Login required\"}");
    return false;
}

void WebServerManager::begin() {
    wifiRequests = xQueueCreate(1, sizeof(WiFiRequest));
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (auth->isUserAuthenticated(request->header("Cookie"))) request->redirect("/dashboard");
        else {
            auto* response = request->beginResponse(200, "text/html; charset=utf-8", auth->getLoginPageHTML());
            response->addHeader("Cache-Control", "no-store");
            request->send(response);
        }
    });
    server.on("/login", HTTP_GET, [this](AsyncWebServerRequest* request) {
        auto* response = request->beginResponse(200, "text/html; charset=utf-8", auth->getLoginPageHTML());
        response->addHeader("Cache-Control", "no-store");
        request->send(response);
    });
    server.on("/logout", HTTP_POST, [this](AsyncWebServerRequest* request) {
        auto* response = request->beginResponse(303);
        response->addHeader("Location", "/login");
        response->addHeader("Cache-Control", "no-store");
        response->addHeader("Set-Cookie", "GrowBoxSession=; Path=/; HttpOnly; SameSite=Lax; Max-Age=0");
        events.close();
        request->send(response);
    });
    server.on("/connect", HTTP_POST, [this](AsyncWebServerRequest* request) { handleConnect(request); });
    server.on("/scan-networks", HTTP_POST, [this](AsyncWebServerRequest* request) {
        // WiFi's asynchronous scan never holds the network task waiting for results.
        if (connecting.load()) { json(request, 409, "{\"error\":\"Wait for the WiFi connection attempt to finish\"}"); return; }
        scanRequested.store(true);
        json(request, 202, "{\"scanning\":true}");
    });
    server.on("/api/networks", HTTP_GET, [this](AsyncWebServerRequest* request) {
        json(request, 200, String("{\"scanning\":") + ((auth->isScanning() || scanRequested.load()) ? "true" : "false") +
            ",\"options\":" + quoteJSON(auth->getNetworksAsOptions()) + "}");
    });
    server.on("/dashboard", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (!auth->isUserAuthenticated(request->header("Cookie"))) { request->redirect("/"); return; }
        asset(request, "text/html; charset=utf-8", WebAssets::app_shell_html, sizeof(WebAssets::app_shell_html));
    });
    server.on("/api/state", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (checkAuthentication(request)) json(request, 200, stateJSON(hardware->snapshot()));
    });
    server.on("/api/network", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (checkAuthentication(request)) json(request, 200, networkJSON());
    });
    server.on("/api/control", HTTP_POST, [this](AsyncWebServerRequest* request) { handleControl(request); });
#if SIMULATION_MODE
    server.on("/simulation", HTTP_POST, [this](AsyncWebServerRequest* request) { handleSimulation(request); });
#endif
    server.on("/api/config", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (checkAuthentication(request)) json(request, 200, SIMULATION_MODE ? "{\"simulation\":true}" : "{\"simulation\":false}");
    });
    server.on("/app-shell.html", HTTP_GET, [](AsyncWebServerRequest* r) {
        asset(r, "text/html; charset=utf-8", WebAssets::app_shell_html, sizeof(WebAssets::app_shell_html));
    });
    server.on("/app.js", HTTP_GET, [](AsyncWebServerRequest* r) {
        asset(r, "application/javascript", WebAssets::app_js, sizeof(WebAssets::app_js));
    });
    server.on("/app.css", HTTP_GET, [](AsyncWebServerRequest* r) {
        asset(r, "text/css", WebAssets::app_css, sizeof(WebAssets::app_css));
    });
    server.on("/sw.js", HTTP_GET, [](AsyncWebServerRequest* r) {
        asset(r, "application/javascript", WebAssets::sw_js, sizeof(WebAssets::sw_js));
    });
    server.on("/manifest.webmanifest", HTTP_GET, [](AsyncWebServerRequest* r) {
        asset(r, "application/manifest+json", WebAssets::manifest_webmanifest, sizeof(WebAssets::manifest_webmanifest));
    });
    server.on("/icon-192.png", HTTP_GET, [](AsyncWebServerRequest* r) {
        asset(r, "image/png", WebAssets::icon_192_png, sizeof(WebAssets::icon_192_png));
    });
    server.on("/icon-512.png", HTTP_GET, [](AsyncWebServerRequest* r) {
        asset(r, "image/png", WebAssets::icon_512_png, sizeof(WebAssets::icon_512_png));
    });
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(204); });
    events.authorizeConnect([this](AsyncWebServerRequest* request) { return auth->isUserAuthenticated(request->header("Cookie")) && events.count() < 4; });
    events.onConnect([this](AsyncEventSourceClient* client) {
        // Always send a complete snapshot on reconnect; old event history is unnecessary.
        const GrowboxState state = hardware->snapshot();
        client->send(stateJSON(state).c_str(), "state", state.revision, 1500);
        client->send(networkJSON().c_str(), "network");
    });
    server.addHandler(&events);
    server.onNotFound([this](AsyncWebServerRequest* request) {
        if (request->url().startsWith("/api/")) { json(request, 404, "{\"error\":\"Unknown API route\"}"); return; }
        // Captive portal probes redirect to setup without returning HTML for API calls.
        request->redirect(auth->isUserAuthenticated(request->header("Cookie")) ? "/dashboard" : "http://192.168.4.1/");
    });
    server.begin();
    dnsServer.start(53, "*", WiFi.softAPIP());
    WiFi.setAutoReconnect(false); // retries are scheduled without blocking the server
    loadSavedWiFi();
    Serial.println("Async HTTP + change-only SSE ready");
}

void WebServerManager::handleConnect(AsyncWebServerRequest* request) {
    if (!request->hasParam("username", true) || !request->hasParam("password", true) ||
        !auth->validateCredentials(request->getParam("username", true)->value().c_str(),
                                   request->getParam("password", true)->value().c_str())) {
        request->send(401, "text/html", "<h1>Invalid credentials</h1><a href='/'>Back to login</a>");
        return;
    }
    String network = request->hasParam("network", true) ? request->getParam("network", true)->value() : "";
    String password = request->hasParam("wifi_password", true) ? request->getParam("wifi_password", true)->value() : "";
    if (network.length() > 32 || password.length() > 64) { json(request, 400, "{\"error\":\"Invalid WiFi parameters\"}"); return; }
    if (network.length()) {
        WiFiRequest credentials{};
        network.toCharArray(credentials.ssid, sizeof(credentials.ssid));
        password.toCharArray(credentials.password, sizeof(credentials.password));
        if (!wifiRequests || xQueueSend(wifiRequests, &credentials, 0) != pdTRUE) {
            json(request, 503, "{\"error\":\"WiFi request queue full; retry\"}"); return;
        }
    }
    const bool remember = request->hasParam("remember", true) && request->getParam("remember", true)->value() == "1";
    const String token = auth->createSession(remember);
    if (token.isEmpty()) { json(request, 503, "{\"error\":\"Could not save sign-in; uncheck Keep me signed in and retry\"}"); return; }
    auto* response = request->beginResponse(303);
    response->addHeader("Location", "/dashboard");
    response->addHeader("Cache-Control", "no-store");
    String cookie = "GrowBoxSession=" + token + "; Path=/; HttpOnly; SameSite=Lax";
    if (remember) cookie += "; Max-Age=2592000";
    if (request->header("X-Forwarded-Proto") == "https") cookie += "; Secure";
    response->addHeader("Set-Cookie", cookie);
    request->send(response);
}

void WebServerManager::handleControl(AsyncWebServerRequest* request) {
    if (!checkAuthentication(request)) return;
    if (!request->hasParam("device", true)) { json(request, 400, "{\"error\":\"Device required\"}"); return; }
    const String device = request->getParam("device", true)->value();
    HardwareCommand command;
    if (device == "pump") command.type = CommandType::TogglePump;
    else if (device == "growLed") command.type = CommandType::ToggleGrowLed;
    else if (device == "rgb") command.type = CommandType::ToggleRGB;
    else if (device == "boost") command.type = CommandType::ToggleBoost;
    else if (device == "brightness") {
        command.type = CommandType::Brightness;
        if (!request->hasParam("value", true) || !parseInteger(request->getParam("value", true)->value(), 100, command.value)) {
            json(request, 400, "{\"error\":\"Brightness must be 0..100\"}"); return;
        }
    } else { json(request, 400, "{\"error\":\"Unknown device\"}"); return; }
    // The actuator task rechecks pump protection when it executes this command.
    if (!hardware->enqueue(command)) { json(request, 503, "{\"error\":\"Control queue full; retry\"}"); return; }
    json(request, 202, "{\"queued\":true}");
}

void WebServerManager::handleSimulation(AsyncWebServerRequest* request) {
    if (!checkAuthentication(request)) return;
    const char* names[] = {"temperature", "humidity", "soil", "water", "lux"};
    float values[5];
    const float minimum[] = {-40, 0, 0, 0, 0};
    const float maximum[] = {80, 100, 100, 100, 100000};
    for (int i = 0; i < 5; ++i) {
        if (!request->hasParam(names[i], true)) { json(request, 400, "{\"error\":\"All simulation values required\"}"); return; }
        const String text = request->getParam(names[i], true)->value();
        char* end = nullptr;
        values[i] = strtof(text.c_str(), &end);
        if (end == text.c_str() || *end || !isfinite(values[i]) || values[i] < minimum[i] || values[i] > maximum[i]) {
            json(request, 400, "{\"error\":\"Invalid simulation value\"}"); return;
        }
    }
    SimulationValues simulation{values[0], values[1], int(values[2]), int(values[3]), values[4]};
    json(request, hardware->simulate(simulation) ? 202 : 503, "{\"queued\":true}");
}

String WebServerManager::stateJSON(const GrowboxState& s) {
    String body;
    body.reserve(420);
    body = "{\"revision\":" + String(s.revision) + ",\"fresh\":" + (s.fresh ? "true" : "false");
    body += ",\"temperature\":" + number(s.sensors.temperature, s.fresh && s.sensors.temperature > -998);
    body += ",\"humidity\":" + number(s.sensors.humidity, s.fresh && s.sensors.humidity >= 0);
    body += ",\"soil\":" + number(s.sensors.soil, s.fresh && s.sensors.soil >= 0, 0);
    body += ",\"water\":" + number(s.sensors.water, s.fresh && s.sensors.water >= 0, 0);
    body += ",\"lux\":" + number(s.sensors.lux, s.fresh && s.sensors.lux >= 0, 0);
    body += String(",\"pump\":") + (s.pump ? "true" : "false");
    body += String(",\"growLed\":") + (s.growLed ? "true" : "false");
    body += String(",\"boost\":") + (s.boost ? "true" : "false");
    body += String(",\"rgb\":") + (s.rgb ? "true" : "false");
    body += ",\"brightness\":" + String(s.brightness);
    body += ",\"soilColor\":" + String(s.soilColor) + ",\"waterColor\":" + String(s.waterColor) + "}";
    return body;
}
String WebServerManager::networkJSON() {
    const bool connected = WiFi.status() == WL_CONNECTED && uint32_t(WiFi.localIP()) != 0;
    const String ip = connected ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
    String body;
    body.reserve(450);
    body = String("{\"connected\":") + (connected ? "true" : "false") +
        ",\"connecting\":" + (connecting.load() ? "true" : "false") +
        ",\"saved\":" + (haveSavedWiFi.load() ? "true" : "false") +
        ",\"saveFailed\":" + (wifiSaveFailed.load() ? "true" : "false") +
        ",\"failed\":" + (wifiAttemptFailed.load() ? "true" : "false") +
        ",\"mdns\":" + (mdnsReady.load() ? "true" : "false");
    body += ",\"ssid\":" + quoteJSON(connected ? WiFi.SSID() : String(""));
    body += ",\"ip\":\"" + ip + "\",\"apIp\":\"" + WiFi.softAPIP().toString() + "\"";
    body += String(",\"hostname\":\"") + GROWBOX_HOSTNAME + ".local\"";
    body += String(",\"dashboardUrl\":\"http://") + GROWBOX_HOSTNAME + ".local/dashboard\"";
    body += ",\"ipUrl\":\"http://" + ip + "/dashboard\"}";
    return body;
}
void WebServerManager::loadSavedWiFi() {
    Preferences preferences;
    if (!preferences.begin("growbox-wifi", true)) return;
    WiFiRequest credentials{};
    const bool valid = preferences.getBytesLength("credentials") == sizeof(credentials) &&
        preferences.getBytes("credentials", &credentials, sizeof(credentials)) == sizeof(credentials) &&
        memchr(credentials.ssid, 0, sizeof(credentials.ssid)) && credentials.ssid[0] &&
        memchr(credentials.password, 0, sizeof(credentials.password));
    preferences.end();
    if (valid) {
        savedWiFi = credentials;
        haveSavedWiFi.store(true);
        connectStarted = millis() - WIFI_RETRY_INTERVAL_MS; // first loop may reconnect immediately
    }
    memset(&credentials, 0, sizeof(credentials));
}
void WebServerManager::startWiFi(const WiFiRequest& credentials, bool saveOnSuccess) {
    activeWiFi = credentials;
    saveActiveWiFi = saveOnSuccess;
    wifiSaveFailed.store(false);
    wifiAttemptFailed.store(false);
    if (mdnsReady.exchange(false)) MDNS.end();
    wasConnected = false;
    WiFi.disconnect(false); // keep setup AP enabled and retain the last working saved configuration
    connecting.store(true);
    connectStarted = millis();
    WiFi.begin(activeWiFi.ssid, activeWiFi.password);
}
void WebServerManager::startMDNS() {
    lastMDNSAttempt = millis();
    if (MDNS.begin(GROWBOX_HOSTNAME)) {
        MDNS.addService("http", "tcp", 80);
        MDNS.addServiceTxt("http", "tcp", "path", "/dashboard");
        mdnsReady.store(true);
        Serial.printf("Home dashboard: http://%s.local (IP fallback: http://%s)\n",
                      GROWBOX_HOSTNAME, WiFi.localIP().toString().c_str());
    } else {
        Serial.println("mDNS unavailable; use the home-network IP shown on the dashboard");
    }
}
void WebServerManager::handleWiFi() {
    auth->pollNetworks();
    if (!connecting.load() && scanRequested.exchange(false)) auth->scanNetworks();
    WiFiRequest credentials{};
    // A scan and a connection attempt must not compete for the radio.
    if (!connecting.load() && !auth->isScanning()) {
        if (wifiRequests && xQueueReceive(wifiRequests, &credentials, 0) == pdTRUE) {
            startWiFi(credentials, true);
        } else if (haveSavedWiFi.load() && (WiFi.status() != WL_CONNECTED || uint32_t(WiFi.localIP()) == 0) &&
                   uint32_t(millis() - connectStarted) >= WIFI_RETRY_INTERVAL_MS) {
            startWiFi(savedWiFi, false);
        }
    }
    memset(&credentials, 0, sizeof(credentials));
    const bool connected = WiFi.status() == WL_CONNECTED && uint32_t(WiFi.localIP()) != 0;
    if (connected && (!connecting.load() || WiFi.SSID() == String(activeWiFi.ssid))) {
        if (connecting.exchange(false)) {
            if (saveActiveWiFi) {
                Preferences preferences;
                bool saved = false;
                if (preferences.begin("growbox-wifi", false)) {
                    saved = preferences.putBytes("credentials", &activeWiFi, sizeof(activeWiFi)) == sizeof(activeWiFi);
                    preferences.end();
                }
                wifiSaveFailed.store(!saved);
                if (saved) { savedWiFi = activeWiFi; haveSavedWiFi.store(true); }
            }
            memset(&activeWiFi, 0, sizeof(activeWiFi));
        }
        if (!wasConnected) {
            wasConnected = true;
            configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
            startMDNS();
        } else if (!mdnsReady.load() && uint32_t(millis() - lastMDNSAttempt) >= WIFI_RETRY_INTERVAL_MS) {
            startMDNS();
        }
    } else {
        if (wasConnected) {
            wasConnected = false;
            if (mdnsReady.exchange(false)) MDNS.end();
        }
        if (connecting.load() && uint32_t(millis() - connectStarted) >= WIFI_CONNECTION_TIMEOUT_MS) {
            connecting.store(false);
            wifiAttemptFailed.store(true);
            memset(&activeWiFi, 0, sizeof(activeWiFi));
            WiFi.disconnect(false); // failed new credentials never overwrite a working saved network
        }
    }
}
void WebServerManager::handleClient() {
    dnsServer.processNextRequest();
    handleWiFi();
    const GrowboxState state = hardware->snapshot();
    if (state.revision != lastRevision) {
        if (events.count()) {
            if (events.send(stateJSON(state).c_str(), "state", state.revision) != AsyncEventSource::ENQUEUED)
                events.close(); // bounded queues: reconnect delivers the latest full state
        }
        lastRevision = state.revision;
    }
    if (uint32_t(millis() - lastNetworkCheck) >= 1000) {
        const String network = networkJSON();
        if (network != lastNetwork) {
            if (events.count()) events.send(network.c_str(), "network");
            lastNetwork = network;
        }
        lastNetworkCheck = millis();
    }
    if (uint32_t(millis() - lastHeartbeat) >= 15000) {
        // Transport heartbeat contains no sensor values and does not change the DOM.
        if (events.count()) events.send("{}", "ping");
        lastHeartbeat = millis();
    }
}
