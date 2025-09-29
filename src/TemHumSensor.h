#ifndef TEMHUMSENSOR_H
#define TEMHUMSENSOR_H

#include "ModbusRegister.h"

class TemHumSensor{
public:
    TemHumSensor(std::shared_ptr<ModbusClient> client, int server_address);

    uint16_t read_tem();
    uint16_t read_hum();

private:
    ModbusRegister rh_register;
    ModbusRegister temp_register;
    uint16_t temperature = 0;
    uint16_t humidity = 0;
};

#endif //TEMHUMSENSOR_H
