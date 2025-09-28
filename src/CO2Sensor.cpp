// CO2Sensor.cpp — no-exceptions build (-fno-exceptions friendly)
// Addressing rule: wire addresses (zero-based). Example: 30001 -> wire 0.

#include "CO2Sensor.h"

// Constructor
// - client: shared Modbus RTU client
// - server_address: Modbus slave ID of the CO₂ sensor
// Registers used here are placeholders; adjust to your device map:
//   IR 30001 (wire 0): CO₂ ppm
//   IR 30002 (wire 1): Temperature (optional)
//   HR 40011 (wire 10): Status word (optional)
CO2Sensor::CO2Sensor(std::shared_ptr<ModbusClient> client, int server_address, int co2_ir_address)
    : co2_register(client, server_address, 0, /*isHolding=*/false),
      temp_ir_(client, server_address, 1, /*isHolding=*/false),
      status_hr_(client, server_address, 10, /*isHolding=*/true) {
    // Initialize cached fields
    last_ppm_   = -1;
    is_working_ = false;
}

// Read CO₂ concentration once.
// Return: ppm (>=0) on success; -1 on error (e.g., Modbus timeout).
int CO2Sensor::readPpm() {
    // ModbusRegister::read() should return an int; negative means failure by convention.
    int v = co2_register.read();
    if (v < 0) {
        last_ppm_ = -1;
        return -1;
    }
    // NOTE: If your device applies scaling (e.g., ppm * 1, 10, or 100), convert here.
    last_ppm_ = v;
    return last_ppm_;
}

// Minimal liveness check by two reads with a short wait.
// If both reads are <= 0, the sensor is considered "not working".
bool CO2Sensor::isWorking() {
    int first = readPpm();
    if (first <= 0) {
        // Keep delay inside the class to mirror your Fan-style API.
        vTaskDelay(pdMS_TO_TICKS(200));  // Increase if your sensor updates slowly
        int second = readPpm();
        if (second <= 0) {
            is_working_ = false;
            return is_working_;
        }
    }
    is_working_ = true;
    return is_working_;
}

// Optional: read raw temperature value (device-specific scaling).
// Return: raw value on success; -100000 on error.
int CO2Sensor::readTemperatureRaw() {
    int v = temp_ir_.read();
    if (v < 0) return -100000;
    return v;
}
