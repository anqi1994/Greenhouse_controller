#ifndef SDP610_H
#define SDP610_H
#include <sys/types.h>
#include <i2c/PicoI2C.h>
#include "ssd1306.h"

class SDP610 {
public:
    SDP610(std::shared_ptr<PicoI2C> i2cbus, uint8_t address=0x40);
    double read();
private:
    std::shared_ptr<PicoI2C> i2c;
    uint8_t addr;
    int16_t read_raw();
};


#endif //SDP610_H
