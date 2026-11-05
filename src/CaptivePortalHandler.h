#pragma once
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include "Config.h"
#include "CaptivePortalPolicy.h"

// Registered before the application routes; only the setup interface is captured.
class CaptivePortalHandler : public AsyncWebHandler {
public:
    static bool onSetupAP(AsyncWebServerRequest* request) {
        return request->client() && request->client()->localIP() == WiFi.softAPIP();
    }
    static void redirectToSetup(AsyncWebServerRequest* request) {
        const String url = String("http://") + WiFi.softAPIP().toString() + "/";
        auto* response = request->beginResponse(302, "text/html; charset=utf-8",
            "<!doctype html><title>GrowBox setup</title><a href=\"" + url + "\">Open GrowBox setup</a>");
        response->addHeader("Location", url); // absolute even for a previously signed-in browser
        response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate");
        response->addHeader("Pragma", "no-cache");
        request->send(response);
    }
    bool canHandle(AsyncWebServerRequest* request) const override {
        const bool readRequest = request->method() == HTTP_GET || request->method() == HTTP_HEAD;
        if (!onSetupAP(request) || !readRequest) return false;
        if (request->url() == "/wpad.dat") return true;
        String host = request->host();
        host.toLowerCase();
        const int port = host.indexOf(':');
        if (port >= 0) host = host.substring(0, port);
        if (host.endsWith(".")) host.remove(host.length() - 1);
        const bool knownHost = host == WiFi.softAPIP().toString() || host == WiFi.localIP().toString() ||
            host == GROWBOX_HOSTNAME || host == String(GROWBOX_HOSTNAME) + ".local";
        return CaptivePortalPolicy::shouldRedirect(true, readRequest, request->url().c_str(), knownHost);
    }
    void handleRequest(AsyncWebServerRequest* request) override {
        if (request->url() == "/wpad.dat") {
            auto* response = request->beginResponse(404, "text/plain", "Not found");
            response->addHeader("Cache-Control", "no-store");
            request->send(response);
        } else redirectToSetup(request);
    }
};
