# GrowBox integration

Current architecture, pins, builds and API are documented in [README.md](README.md).

Two FreeRTOS tasks own sensors and actuators. Async HTTP reads cached state and queues commands. The dashboard receives server-sent events without page reloads.

The current hardware is ESP32-S3 with SHT40 and Grove I2C water sensing. The old DHT22 pin list and 80% pump cutoff are obsolete. Current pins are in src/Config.h; the unified soil stop threshold is 60%.

PWA installation/offline support requires HTTPS or localhost. Live updates work over ordinary ESP32 HTTP. The included local proxy provides a PC installation path.
