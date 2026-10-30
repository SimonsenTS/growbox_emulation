# GrowBox: FreeRTOS hardware control and live PWA

ESP32-S3 firmware for SHT40, powered analog soil moisture, Grove I2C water level, pump relay, grow LED/PWM/boost, two WS2812B LEDs, and an optional LTR-303 lux sensor.

## Analysis and changes

Previously, sensor acquisition, automation and HTTP shared the main loop. Each soil measurement waited 50 + 150 + 50 ms. Water reads could spin for 100 ms per I2C address. The dashboard repeated measurements and hardware writes inside its HTTP handler. Controls redirected to another page. Pump rules conflicted: the loop stopped at 60% soil while the dashboard used 80%.

ESP32 Arduino already uses FreeRTOS. Hardware now has explicit tasks and a single owner per peripheral:

| Work | Owner | Schedule |
| --- | --- | --- |
| SHT40, soil power/ADC, Grove water, optional LTR-303 | Sensor task | Every second, priority 1, core 1 |
| Relays, grow LED/PWM/boost, RGB, button, pump protection | Actuator task | Queue wakeup or 20 ms timeout, priority 2, core 1 |
| HTTP and SSE | ESPAsyncWebServer / AsyncTCP | Asynchronous handlers |
| DNS, WiFi progress, event publication | Arduino loop | Yields every 2 ms |

The sensor task's settling delays yield to other tasks. One sensor task serializes the shared I2C bus and power sequencing. A separate actuator task keeps buttons and commands responsive during sensor waits.

A one-sample mailbox transfers complete readings; a bounded 16-command queue transfers controls. A short critical section protects a plain-data snapshot. No lock spans sensor delays, I2C, HTTP or LED operations. HTTP 202 acknowledges queued work; subsequent events report the applied device state.

The server sends a full JSON snapshot only when a displayed value or device state changes. Temperature/humidity are rounded to 0.1 and lux to whole units. The browser updates only affected DOM fields. A new/reconnected client receives the latest complete snapshot. A 15-second transport heartbeat carries no readings. There is no periodic sensor polling or dashboard reload. RGB outputs are also written only when their selected color or enable setting changes.

Up to four SSE clients are admitted, with eight queued messages per client. If publication cannot be queued, streams reconnect and receive the latest state rather than accumulating unlimited messages.

## Pump behaviour

- Auto-start below 20% soil moisture when water is above 10%.
- Continue through the intermediate band; stop at 60% soil.
- Stop/block starts at water <=10%, invalid soil/water readings, or samples older than four seconds.
- Manual OFF lasts until the next automatic sample, preserving the original automatic behaviour.
- Missing/partial Grove I2C replies are unavailable and lock the pump. Missing SHT40/lux measurements display as unavailable. A disconnected analog soil probe cannot be reliably detected from ADC voltage alone.
- Critical water stays red even when status LEDs are disabled.

Sampling and protection continue without a browser. Physical response times and relay operation require bench validation.

## Build and upload

```sh
pio run -e esp32
pio run -e esp32 -t upload
pio device monitor -b 115200
```

The default environment is esp32. Its image is .pio/build/esp32/firmware.bin. The build compresses web/ into firmware using scripts/embed_web.py; no filesystem upload is needed. Tested with Espressif32 8.0.0 / Arduino-ESP32 3.3.12.

Connect to the open **GrowBox_Setup** AP and visit **http://192.168.4.1**. Default login: admin / password123. Select your home 2.4 GHz network or use setup WiFi only. Network scans and connection attempts are asynchronous. Failed home WiFi leaves the setup AP available.

## Home WiFi and a named dashboard

1. Join GrowBox_Setup on your phone or computer and open http://192.168.4.1.
2. Select your router's 2.4 GHz WiFi, enter its password, and log in.
3. Wait for the dashboard to show Home WiFi connected and the home-network links.
4. Return your phone or computer to your normal router WiFi in the device's WiFi settings.
5. Open **http://growbox-dashboard.local**. The dashboard and ordinary internet browsing now use your home network.

The ESP32 stays connected to the router while its setup AP remains available. Your phone/computer can use the router's 2.4 or 5 GHz network if both bands share the same LAN. The webpage cannot switch the phone/computer's WiFi connection automatically.

The hostname is GROWBOX_HOSTNAME in src/Config.h. Its default is growbox-dashboard; mDNS resolves growbox-dashboard.local on the local network. Use the complete http:// address to avoid a browser search. If multicast/mDNS is unavailable, use the home-network IP link displayed during setup or find GrowBox in the router's client list. Guest/client isolation can prevent local device access.

Successful WiFi credentials are saved in the ESP32's NVS and retried after restart or connection loss. Attempts time out after 20 seconds and saved credentials are retried at 30-second intervals. A failed new configuration preserves the last working saved network. Saving failures are reported on the dashboard. Credentials stay on the device and are not included in the network status API.

Remembered login cookies belong to the address used: the first visit to growbox-dashboard.local needs a login even if you already logged in at 192.168.4.1. Keep me signed in remembers the named address thereafter. This URL works on the same LAN; PWA installation still uses the HTTPS/localhost requirements below.

Pins/calibration remain in src/Config.h. Confirm them against your actual ESP32-S3 module. The checked-in diagram.json is an older ESP32/DHT22 prototype, not a validated wiring diagram for the current firmware.

## Lux sensor

The original source only reserved an LTR-303 for PCB v2; no lux driver existed. New optional support shares SDA 18 / SCL 17, address 0x29:

```sh
pio run -e lux
pio run -e lux -t upload
```

This sets ENABLE_LUX_SENSOR=1. The default esp32 build leaves it disabled for the existing PCB. The driver uses gain 1, 100 ms integration, a 500 ms measurement rate and the LTR-303 datasheet formula. Missing, failed and saturated measurements display as unavailable. Validate gain/calibration on real hardware.

## PWA installation

Live updates work on ordinary ESP32 HTTP without reloading. Installation and service workers additionally require trusted HTTPS or localhost; a private LAN address over HTTP does not qualify. See [MDN installation requirements](https://developer.mozilla.org/en-US/docs/Web/Progressive_web_apps/Guides/Making_PWAs_installable).

For a PC installation/test path, connect the PC to GrowBox and run:

```sh
node scripts/local_proxy.cjs http://192.168.4.1
```

Open **http://localhost:8180** in Edge/Chrome, log in, and use Install app when offered or the browser installation menu. This proxy streams SSE and preserves API paths. Keep it running at the same address for live access from the installed app. It binds only to the PC's loopback interface.

Localhost on a phone refers to that phone. For phone installation use a trusted HTTPS reverse proxy on your LAN, forwarding /events without buffering and with a timeout longer than the 15-second heartbeat. HTTPS hosting/certificate setup is not included in the firmware.

The service worker caches only public shell assets, never login responses, readings or commands. Offline opening shows the shell with controls disabled. Commands are not queued for later replay. Reconnection gets current state. Assets revalidate online; change CACHE in web/sw.js when changing the cached asset list.

The login page offers **Keep me signed in for 30 days** (checked by default), password-manager autofill and Sign out. A signed HttpOnly cookie remembers each browser, including across ESP32 restarts. Only a random signing key is saved in ESP32 NVS; the password is not saved in browser storage. Changing the firmware credentials invalidates existing tokens.

With NTP, the server checks token expiry; without a valid device clock, the browser enforces the 30-day cookie lifetime. Unchecked sign-in uses a session cookie and a boot-specific key. Remembered sign-in is tied to the hostname used: device IP and localhost are separate. The setup AP remains open and defaults unchanged; use a trusted local network.

## Files and API

- src/HardwareRuntime.*: task ownership, queues, snapshot and revision.
- src/ControlPolicy.h: pump thresholds and lux conversion.
- src/SensorManager.*: sensor operations and bounded I2C reads.
- src/DeviceController.*: outputs and button.
- src/WebServerManager.*: async API, SSE, queued WiFi configuration.
- web/: responsive dashboard, manifest, service worker and icons.
- scripts/embed_web.py: assets embedded in flash.
- scripts/local_proxy.cjs: optional PC localhost access.

| Endpoint | Purpose |
| --- | --- |
| GET /dashboard | Authenticated dashboard |
| GET /api/state | Cached state, no sensor acquisition |
| GET /api/network | Authenticated home WiFi status and dashboard/IP links; no credentials |
| GET /events | Authenticated change-only SSE stream |
| POST /api/control | device=pump, growLed, boost, rgb, or brightness with value=0..100 |
| GET /api/config | Simulation panel availability |
| POST /simulation | All five simulated values; simulation builds only |
| POST /connect | Login and optional queued WiFi configuration |
| GET /login, POST /logout | Login form and clearing remembered sign-in |
| POST /scan-networks, GET /api/networks | Async scan and results |

Hardware APIs return JSON 401 for missing login, 400 for malformed controls and 503 when queues are full. Legacy GET toggle/brightness routes are replaced by POST controls.

## Verification

Compile all environments with:

```sh
pio run -e esp32 -e simulation -e lux
```

Compile-time checks exercise the actual pump/lux functions at threshold boundaries, with missing/stale data, darkness and saturation:

```sh
xtensa-esp32s3-elf-g++ -std=c++17 -fsyntax-only -Isrc tests/control_policy.cpp
```

Browser tests use actual assets with mock firmware HTTP/SSE transport, covering changed-field DOM updates, controls, simulation, reconnects, home-WiFi handoff and IP fallback, errors, mobile overflow and offline/cache isolation. They do not emulate FreeRTOS, physical sensors, radio reconnection, NVS retention or mDNS resolution:

```sh
npm --cache .pio/npm-cache install --prefix .pio/browser-tests --no-save --package-lock=false playwright
node --test tests/browser.test.cjs
```

Tests use installed Microsoft Edge. Dependencies/screenshots stay under ignored .pio/. Firmware and the optional proxy need no npm packages.

Before putting the firmware into service, verify relay polarity, sensor calibration, wiring, command/button responsiveness during sampling, and pump shutoff when the water sensor is disconnected. Also check home-WiFi provisioning, saved credentials across power cycles, router reconnection, and name resolution from your phone/computer. No physical device was flashed or benchmarked during this change.
