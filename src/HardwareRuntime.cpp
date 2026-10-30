#include "HardwareRuntime.h"
#include "ControlPolicy.h"
#include <math.h>

static_assert(AUTO_SENSOR_INTERVAL > 0, "Periodic sampling is required for pump protection");
static_assert(SENSOR_STALE_MS > AUTO_SENSOR_INTERVAL, "Stale timeout must exceed sampling period");

bool HardwareRuntime::begin() {
    devices.begin(); // outputs OFF before any task can run
    samples = xQueueCreate(1, sizeof(SensorSample));
    commands = xQueueCreate(16, sizeof(HardwareCommand));
    simulation = xQueueCreate(1, sizeof(SimulationValues));
    if (!samples || !commands || !simulation) return false;
    TaskHandle_t control = nullptr;
    // Arduino already runs on FreeRTOS. AsyncTCP handles networking separately.
    if (xTaskCreatePinnedToCore(controlEntry, "actuators", 4096, this, 2, &control, 1) != pdPASS)
        return false;
    if (xTaskCreatePinnedToCore(sensorEntry, "sensors", 6144, this, 1, nullptr, 1) != pdPASS) {
        vTaskDelete(control);
        devices.setPumpState(false);
        return false;
    }
    return true;
}
bool HardwareRuntime::enqueue(HardwareCommand command) {
    return commands && xQueueSend(commands, &command, 0) == pdTRUE;
}
bool HardwareRuntime::simulate(const SimulationValues& values) {
#if SIMULATION_MODE
    return simulation && xQueueOverwrite(simulation, &values) == pdTRUE;
#else
    return false;
#endif
}
GrowboxState HardwareRuntime::snapshot() {
    portENTER_CRITICAL(&stateLock);
    GrowboxState copy = state;
    portEXIT_CRITICAL(&stateLock);
    return copy;
}
void HardwareRuntime::sensorEntry(void* context) {
    static_cast<HardwareRuntime*>(context)->sensorLoop();
}
void HardwareRuntime::controlEntry(void* context) {
    static_cast<HardwareRuntime*>(context)->controlLoop();
}
void HardwareRuntime::sensorLoop() {
    sensors.begin(); // this task is the only owner of Wire and sensor power
    TickType_t next = xTaskGetTickCount();
    for (;;) {
#if SIMULATION_MODE
        SimulationValues values;
        if (xQueueReceive(simulation, &values, 0) == pdTRUE) {
            sensors.setSimulatedTemperature(values.temperature);
            sensors.setSimulatedHumidity(values.humidity);
            sensors.setSimulatedSoilPercentage(values.soil);
            sensors.setSimulatedWaterPercentage(values.water);
            sensors.setSimulatedLux(values.lux);
        }
#endif
        SensorSample sample;
        sensors.measureSHT40();
        sample.temperature = sensors.readTemperature();
        sample.humidity = sensors.readHumidity();
        sample.water = sensors.getWaterPercentage(); // water before powered soil
        sample.soil = sensors.getSoilPercentage();
        sample.lux = sensors.readLux();
        sample.sampledAt = millis();
        xQueueOverwrite(samples, &sample);
        vTaskDelayUntil(&next, pdMS_TO_TICKS(AUTO_SENSOR_INTERVAL));
    }
}
void HardwareRuntime::controlLoop() {
    SensorSample sample;
    bool haveSample = false;
    for (;;) {
        HardwareCommand command;
        const bool gotCommand = xQueueReceive(commands, &command, pdMS_TO_TICKS(CONTROL_INTERVAL_MS)) == pdTRUE;
        const bool newSample = xQueueReceive(samples, &sample, 0) == pdTRUE;
        haveSample = haveSample || newSample;
        const bool fresh = haveSample && uint32_t(millis() - sample.sampledAt) <= SENSOR_STALE_MS;
        const bool pumpAllowed = ControlPolicy::pumpAllowed(sample.soil, sample.water, fresh);
        if (gotCommand) {
            switch (command.type) {
                case CommandType::TogglePump:
                    devices.setPumpState(!devices.getPumpState() && pumpAllowed); break;
                case CommandType::ToggleGrowLed: devices.toggleGrowLed(); break;
                case CommandType::ToggleRGB: devices.toggleRGBLeds(); break;
                case CommandType::ToggleBoost: devices.toggleGrowLedBoost(); break;
                case CommandType::Brightness:
                    if (command.value > 0 && !devices.getGrowLedState()) devices.setGrowLedState(true);
                    devices.updateGrowLEDBrightness(constrain(command.value, 0, 100)); break;
            }
        }
        if (devices.checkButton()) devices.setPumpState(!devices.getPumpState() && pumpAllowed);
        if (newSample) {
            const bool running = ControlPolicy::automaticPump(devices.getPumpState(), sample.soil, sample.water, fresh);
            if (running != devices.getPumpState()) devices.setPumpState(running);
        }
        // Safety is enforced every tick, even without a new sample.
        if (!pumpAllowed && devices.getPumpState()) devices.setPumpState(false);
        devices.updateSoilMoistureColor(fresh ? sample.soil : -1);
        devices.updateWaterLevelColor(fresh ? sample.water : -1);
        publish(sample, fresh);
    }
}
void HardwareRuntime::publish(const SensorSample& sample, bool fresh) {
    GrowboxState next;
    next.sensors = sample;
    // Publish at display precision to avoid transmitting invisible float changes.
    next.sensors.temperature = roundf(sample.temperature * 10) / 10;
    next.sensors.humidity = roundf(sample.humidity * 10) / 10;
    next.sensors.lux = roundf(sample.lux);
    next.fresh = fresh;
    next.pump = devices.getPumpState();
    next.growLed = devices.getGrowLedState();
    next.boost = devices.getGrowLedBoostState();
    next.rgb = devices.getRGBLedsEnabled();
    next.brightness = devices.getBrightness();
    next.soilColor = (devices.getSoilRedValue() << 16) | (devices.getSoilGreenValue() << 8) | devices.getSoilBlueValue();
    next.waterColor = (devices.getWaterRedValue() << 16) | (devices.getWaterGreenValue() << 8) | devices.getWaterBlueValue();
    portENTER_CRITICAL(&stateLock);
    const bool changed = next.sensors.temperature != state.sensors.temperature ||
        next.sensors.humidity != state.sensors.humidity || next.sensors.lux != state.sensors.lux ||
        next.sensors.soil != state.sensors.soil || next.sensors.water != state.sensors.water ||
        next.fresh != state.fresh || next.pump != state.pump || next.growLed != state.growLed ||
        next.boost != state.boost || next.rgb != state.rgb || next.brightness != state.brightness ||
        next.soilColor != state.soilColor || next.waterColor != state.waterColor;
    next.revision = state.revision + (changed ? 1 : 0);
    state = next;
    portEXIT_CRITICAL(&stateLock);
}
