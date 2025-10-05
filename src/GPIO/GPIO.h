#ifndef GPIO_H
#define GPIO_H

#include "pico/stdlib.h"

class GPIO{
public:
    // default : input mode, pullup true, invert true
    GPIO(uint8_t pin, bool input=true, bool pullup=true, bool invert=true);
    GPIO(const GPIO &) = delete;

    bool read() const;
    void write(bool value);
    operator uint() const;


private:
    uint pin_number;
    bool is_input;
    bool is_pullup;
    bool is_inverted;
};

#endif //GPIO_H
