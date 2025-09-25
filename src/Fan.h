#ifndef FAN_H
#define FAN_H

#include "modbus/ModbusRegister.h"

class Fan{
public:
    Fan(std::shared_ptr<ModbusClient> client, int server_address);

    void setSpeed(int value);
    bool isWorking();
    bool checkWorkingStatus() const;
    int getSpeed() const;

private:
    ModbusRegister produal_speed;
    ModbusRegister produal_pulse;
    int current_speed = 0;
    bool is_working = true;
};

#endif //FAN_H
