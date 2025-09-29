#ifndef VALVE_H
#define VALVE_H
#include "GPIO/GPIO.h"

class Valve{
public:

    Valve(uint8_t pin);

    void open();
    void close();
    bool check_open() const;

private:
    GPIO gpio;
    bool opened;
};

#endif //VALVE_H
