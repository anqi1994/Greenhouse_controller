#ifndef TEMHUMSENSOR_H
#define TEMHUMSENSOR_H

#include "ModbusRegister.h"

class HMP60{
public:
    HMP60(std::shared_ptr<ModbusClient> client, int server_address);

    double read_tem();
    double read_hum();

private:
    ModbusRegister rh_register;
    ModbusRegister temp_register;
    double temperature = 0;
    double humidity = 0;
};

#endif //TEMHUMSENSOR_H
