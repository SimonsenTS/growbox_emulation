// Compile-time checks execute the actual policy functions at threshold boundaries.
#include "ControlPolicy.h"
using namespace ControlPolicy;
static_assert(automaticPump(false, 19, 11, true), "dry soil starts pump");
static_assert(!automaticPump(false, 20, 11, true), "20 percent does not start");
static_assert(automaticPump(true, 59, 11, true), "pump continues through hysteresis band");
static_assert(!automaticPump(true, 60, 11, true), "60 percent stops pump");
static_assert(!automaticPump(true, 19, 10, true), "10 percent water stops pump");
static_assert(!automaticPump(false, 19, 0, true), "empty tank blocks auto-start");
static_assert(!pumpAllowed(-1, 80, true), "missing soil locks pump");
static_assert(!pumpAllowed(19, -1, true), "missing water locks pump");
static_assert(!pumpAllowed(19, 80, false), "stale readings lock manual control");
static_assert(!automaticPump(true, 19, 80, false), "stale readings stop running pump");
static_assert(pumpAllowed(20, 11, true), "manual start allowed in hysteresis band");
static_assert(lux(0, 0) == 0, "darkness is valid zero lux");
static_assert(lux(65535, 0) < 0, "saturation is unavailable");
static_assert(lux(100, 0) > 177 && lux(100, 0) < 178, "low ratio lux");
static_assert(lux(100, 100) > 232 && lux(100, 100) < 233, "mid ratio lux");
static_assert(lux(25, 75) > 23 && lux(25, 75) < 24, "high ratio lux");
static_assert(lux(1, 99) == 0, "infrared dominated reading");
