#pragma once
#include <stdint.h>

namespace ControlPolicy {
constexpr bool pumpAllowed(int soil, int water, bool fresh) {
    return fresh && soil >= 0 && soil < 60 && water > 10;
}
constexpr bool automaticPump(bool running, int soil, int water, bool fresh) {
    if (!pumpAllowed(soil, water, fresh)) return false;
    return running || soil < 20;
}

// LTR-303 datasheet formula, fixed gain=1 and integration=100 ms.
constexpr float lux(uint16_t ch0, uint16_t ch1) {
    if (ch0 == 65535 || ch1 == 65535) return -1; // saturated
    if (ch0 == 0 && ch1 == 0) return 0;
    const float ratio = float(ch1) / (float(ch0) + ch1);
    float value = 0;
    if (ratio < 0.45f) value = 1.7743f * ch0 + 1.1059f * ch1;
    else if (ratio < 0.64f) value = 4.2785f * ch0 - 1.9548f * ch1;
    else if (ratio < 0.85f) value = 0.5926f * ch0 + 0.1185f * ch1;
    return value > 0 ? value : 0;
}
}
