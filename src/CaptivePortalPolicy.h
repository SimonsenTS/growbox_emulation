#pragma once

namespace CaptivePortalPolicy {
constexpr const char* probePaths[] = {
    "/connecttest.txt", "/ncsi.txt", "/redirect", "/fwlink",
    "/generate_204", "/gen_204", "/hotspot-detect.html",
    "/library/test/success.html", "/success.txt", "/canonical.html",
    "/connectivity-check.html", "/check_network_status.txt"
};
constexpr bool equal(const char* first, const char* second) {
    while (*first && *first == *second) { ++first; ++second; }
    return *first == *second;
}
constexpr bool isProbe(const char* path) {
    for (const auto* probe : probePaths) if (equal(path, probe)) return true;
    return false;
}
constexpr bool isAPI(const char* path) {
    return path[0] == '/' && path[1] == 'a' && path[2] == 'p' && path[3] == 'i' && path[4] == '/';
}
constexpr bool shouldRedirect(bool onSetupAP, bool readRequest, const char* path, bool knownHost) {
    return onSetupAP && readRequest && !isAPI(path) && (isProbe(path) || !knownHost);
}
}
