# GrowBox simulation

```sh
pio run -e simulation
pio run -e simulation -t upload
```

This enables SIMULATION_MODE and the simulation panel. Physical sensor acquisition is bypassed. **Actuator GPIO still operates normally, including the pump relay.** Simulation values exercise the real control policy.

Defaults: 25 C, 50% humidity, 30% soil, 70% water and 500 lux. Apply all five values; the sensor task consumes them on its next one-second cycle. Readings and outputs update through SSE without reloading.

Try soil 19% / water 70% for automatic start, soil 60% for stop, and water 10% to stop/block the pump. Manual OFF in dry soil lasts until the next automatic sample. Critical water remains red with status LEDs disabled.

POST /simulation accepts URL-encoded temperature, humidity, soil, water and lux. HTTP 202 acknowledges queued values. The route is absent from production builds.

The existing diagram.json and wokwi.toml describe an older ESP32/DHT22 prototype and have not been validated for current ESP32-S3 wiring. Building this environment does not validate that diagram. Browser-only tests need no connected board; see [README.md](README.md).
