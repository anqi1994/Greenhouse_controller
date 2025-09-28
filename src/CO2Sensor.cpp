//
// Created by An Qi on 28.9.2025.
//

#include "CO2Sensor.h"

// Constructor
// Comment: Default wire addresses used here are common placeholders.
// Replace them with your device's real register map if needed.
CO2Sensor::CO2Sensor(std::shared_ptr<ModbusClient> client, int server_address)
    : co2_ppm_(client, server_address, 0, /*isHolding=*/false),   // IR 30001: CO2 ppm
      temp_ir_(client, server_address, 1, /*isHolding=*/false),   // IR 30002: Temperature (optional)
      status_hr_(client, server_address, 10, /*isHolding=*/true)  // HR 40011: Status word (optional)
{
    last_ppm_   = -1;
    is_working_ = false;
}

// Read CO₂ ppm once.
// Return ppm >= 0 on success; -1 on error (e.g., Modbus timeout/exception).
int CO2Sensor::readPpm() {
    try {
        int v = co2_ppm_.read();
        if (v < 0) {
            last_ppm_ = -1;
            return -1;
        }
        last_ppm_ = v; // NOTE: some devices scale ppm or use signed/unsigned; adjust if needed.
        return last_ppm_;
    } catch (...) {
        last_ppm_ = -1;
        return -1;
    }
}

// Minimal liveness check by two reads with a short wait.
// If both reads are <= 0, mark not working; otherwise mark working.
// NOTE: Keep vTaskDelay to mirror your Fan class style.
bool CO2Sensor::isWorking() {
    int first = readPpm();
    if (first <= 0) {
        vTaskDelay(pdMS_TO_TICKS(200)); // widen if your sensor updates slowly
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
// Return raw value on success; -100000 on error.
int CO2Sensor::readTemperatureRaw() {
    try {
        int v = temp_ir_.read();
        if (v < 0) return -100000;
        return v;
    } catch (...) {
        return -100000;
    }
}

