#pragma once
#include "SensorManager.h"
#include "DeviceController.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

struct SensorSample {
    float temperature = -999;
    float humidity = -999;
    float lux = -1;
    int soil = -1;
    int water = -1;
    uint32_t sampledAt = 0;
};
struct GrowboxState {
    SensorSample sensors;
    bool fresh = false;
    bool pump = false;
    bool growLed = false;
    bool boost = false;
    bool rgb = true;
    int brightness = 0;
    uint32_t soilColor = 0;
    uint32_t waterColor = 0;
    uint32_t revision = 0;
};
enum class CommandType { TogglePump, ToggleGrowLed, ToggleRGB, ToggleBoost, Brightness };
struct HardwareCommand { CommandType type; int value = 0; };
struct SimulationValues { float temperature; float humidity; int soil; int water; float lux; };

class HardwareRuntime {
public:
    bool begin();
    bool enqueue(HardwareCommand command);
    bool simulate(const SimulationValues& values);
    GrowboxState snapshot();
private:
    SensorManager sensors;
    DeviceController devices;
    QueueHandle_t samples = nullptr;
    QueueHandle_t commands = nullptr;
    QueueHandle_t simulation = nullptr;
    portMUX_TYPE stateLock = portMUX_INITIALIZER_UNLOCKED;
    GrowboxState state;
    static void sensorEntry(void* context);
    static void controlEntry(void* context);
    void sensorLoop();
    void controlLoop();
    void publish(const SensorSample& sample, bool fresh);
};
