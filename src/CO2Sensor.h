//
// Created by An Qi on 28.9.2025.
//

#ifndef CO2SENSOR_H
#define CO2SENSOR_H

#endif //CO2SENSOR_H

#pragma once
#include <memory>
#include "modbus/ModbusClient.h"
#include "ModbusRegister.h"
#include "FreeRTOS.h"
#include "task.h"

// CO2Sensor: minimal CO₂ sensor wrapper using Modbus registers.
// Addressing rule: "wire addresses" (zero-based). E.g. 30001 -> wire 0.
class CO2Sensor {
public:
    // Constructor
    //  - client: shared Modbus RTU client
    //  - server_address: Modbus slave ID of the CO₂ sensor
    CO2Sensor(std::shared_ptr<ModbusClient> client, int server_address, int co2_ir_addr);

    // Read current CO₂ concentration in ppm.
    // Return: ppm value on success; -1 on error.
    int readPpm();

    // Simple working check: read twice with a short delay.
    // If both reads are <= 0, we consider the sensor "not working".
    bool isWorking();

    // Get last ppm value cached by readPpm()/isWorking().
    int getLastPpm() const { return last_ppm_; }

    // Get last working status decided by isWorking().
    bool checkWorkingStatus() const { return is_working_; }

    // Optional: read temperature in Celsius if your device supports it.
    // Return: temperature * 0.1 or real °C depending on your register map; -100000 on error.
    int readTemperatureRaw(); // keep "raw" because scaling varies by device

private:
    // Registers (wire addresses, zero-based). Adjust them to your device map:
    //  - co2_ppm_: Input Register at wire 0 -> 30001
    //  - temp_ir_: Input Register at wire 1 -> 30002 (optional)
    //  - status_hr_: Holding Register at wire 10 -> 40011 (optional, not used in logic here)
    ModbusRegister co2_register;
    ModbusRegister temp_ir_;
    ModbusRegister status_hr_;

    int  last_ppm_{-1};
    bool is_working_{false};
};
