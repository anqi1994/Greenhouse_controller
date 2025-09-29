#ifndef SDP610_H
#define SDP610_H
#include <sys/types.h>
#include <i2c/PicoI2C.h>
#include "ssd1306.h"

class SDP610 {
public:
    SDP610();
    uint read();
    void printHex(int16_t value);
    void scanI2C();
private:
    std::shared_ptr<PicoI2C> i2cbus;
    uint8_t addr = 0x40;
};


#endif //SDP610_H
