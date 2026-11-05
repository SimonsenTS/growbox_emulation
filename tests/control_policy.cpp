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

static_assert(lightPercentage(0, 0, 10000) == 0, "minimum is zero percent");
static_assert(lightPercentage(5000, 0, 10000) == 50, "midpoint is fifty percent");
static_assert(lightPercentage(10000, 0, 10000) == 100, "maximum is one hundred percent");
static_assert(lightPercentage(20000, 0, 10000) == 100, "above maximum clamps");
static_assert(lightPercentage(50, 100, 1100) == 0, "below custom minimum clamps");
static_assert(lightPercentage(600, 100, 1100) == 50, "nonzero minimum is subtracted");
static_assert(lightPercentage(615, 0, 10000) == 6, "nearest whole percentage");
static_assert(lightPercentage(-1, 0, 10000) == -1, "missing sensor stays unavailable");
static_assert(lightPercentage(std::numeric_limits<float>::quiet_NaN(), 0, 10000) == -1, "NaN unavailable");
static_assert(lightPercentage(std::numeric_limits<float>::infinity(), 0, 10000) == -1, "infinity unavailable");
static_assert(lightPercentage(100, 100, 100) == -1, "equal endpoints rejected");
static_assert(lightPercentage(100, 1000, 0) == -1, "reversed endpoints rejected");

static_assert(lightPercentage(725, 0, 1450) == 50, "office light midpoint");
static_assert(lightPercentage(1450, 0, 1450) == 100, "office light maximum");
static_assert(lightPercentage(1800, 0, 1450) == 100, "brighter than office maximum clamps");

#include "CaptivePortalPolicy.h"
using namespace CaptivePortalPolicy;
static_assert(isProbe("/generate_204"), "Android probe");
static_assert(isProbe("/hotspot-detect.html"), "Apple probe");
static_assert(isProbe("/redirect"), "Windows browser launch");
static_assert(shouldRedirect(true, true, "/connecttest.txt", true), "probes stay captive even at canonical host");
static_assert(shouldRedirect(true, true, "/", false), "foreign host root must canonicalize");
static_assert(!shouldRedirect(false, true, "/redirect", false), "never capture home-network requests");
static_assert(!shouldRedirect(true, true, "/api/state", false), "API keeps JSON authentication");
static_assert(!shouldRedirect(true, false, "/connect", false), "do not redirect POST bodies");
static_assert(!shouldRedirect(true, true, "/dashboard", true), "canonical dashboard has no redirect loop");
