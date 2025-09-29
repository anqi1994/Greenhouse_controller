#ifndef FAN_H
#define FAN_H

#include "modbus/ModbusRegister.h"

class Fan{
public:
    Fan(std::shared_ptr<ModbusClient> client, int server_address);

    void setSpeed(int value);
    uint16_t returnPulse();
    int getSpeed() const;

private:
    ModbusRegister produal_speed;
    ModbusRegister produal_pulse;
    int current_speed_in_percent = 0;
};

#endif //FAN_H