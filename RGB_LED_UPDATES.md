# RGB status LEDs

The actuator task exclusively owns both WS2812B outputs. It writes colors only when selected colors change and reapplies them when the enable setting changes.

| Reading | Red | Orange | Green |
| --- | --- | --- | --- |
| Soil | Below 20% or unavailable/stale | 20% to below 60% | 60% and above |
| Water | 10% and below or unavailable/stale | Above 10% to 50% | Above 50% |

Critical water remains red when status LEDs are disabled. The dashboard shows effective colors through SSE. Color and pump rules share the 60% soil / 10% water boundaries. See [README.md](README.md).
