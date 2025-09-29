//
// Created by An Qi on 28.9.2025.
//

#ifndef CO2SENSOR_H
#define CO2SENSOR_H

#endif //CO2SENSOR_H

#pragma once
#include <memory>
#include <cstdint>
#include "FreeRTOS.h"
#include "task.h"
#include "ModbusRegister.h"


class GMP252{
public:
    GMP252(std::shared_ptr<ModbusClient> client, int server_address);

    uint16_t read_value();

private:
    ModbusRegister CO2_read_Register;
    uint16_t read_CO2_value = 0;
};


// Lightweight GMP252 CO2 reader.
// Default register is IR 30258 (wire=257), which is scaled by 10 (raw/10 -> ppm).
/*class CO2Sensor {
public:
    // client: shared ModbusClient transport
    // slave : Modbus slave id (default 240 for GMP252)
    // use_scaled: true -> IR30258 (wire 257, raw/10); false -> IR30257 (wire 256, raw)
    // min_interval_ms: minimal interval between real Modbus reads; within the interval
    //                  the method returns the last value to avoid bus flooding.
    CO2Sensor(std::shared_ptr<ModbusClient> client,
              int slave = 240,
              bool use_scaled = true);

    // Read CO2 in ppm.
    // Returns last successful value on failure.
    // Simple retry & debouncing are applied.
    int readPpm();

    // Last raw register value (for debugging).
    int lastRaw() const { return last_raw_; }

    // Last successful ppm value.
    int lastPpm() const { return last_ppm_; }

    // Whether the last read succeeded.
    bool isWorking() const { return last_ok_; }

    // Switch between scaled and integer register (effective from the next read).
    void setUseScaled(bool use_scaled);

private:
    std::shared_ptr<ModbusClient> client_;
    int  slave_;
    bool scaled_;
    ModbusRegister reg_;            // Bound input register (wire 256/257, isHolding=false)

    // State
    int last_ppm_  = 0;
    int last_raw_  = -1;
    bool last_ok_  = false;
};*/


