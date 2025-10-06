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

