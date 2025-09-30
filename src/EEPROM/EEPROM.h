#ifndef EEPROM_H
#define EEPROM_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <cstring>
#include <string>

class EEPROM {
public:
    EEPROM(i2c_inst_t* i2c, uint sdaPin, uint sclPin);

    bool eepromWrite(uint16_t address, const uint8_t *data, size_t data_len);
    bool eepromRead(uint16_t address, uint8_t *data, size_t data_len);
    uint16_t crc16(const uint8_t *buffer_p, size_t buffer_len);
    bool validate_crc(uint8_t *data_buffer, size_t buffer_len);
    bool writeStatus(const uint16_t address, const char *status);
    bool readStatus(const uint16_t address, char *status_buffer, size_t buffer_len);
    bool readStatus(const uint16_t address, std::string& status_buffer);



private:
    static constexpr uint8_t EEPROM_ADDRESS = 0X50;
    static constexpr uint8_t BUFFER_SIZE = 32;
    static constexpr uint I2C_BAUDRATE = 100000; // 100kHz baudrate for eeprom
    //static constexpr uint16_t STATUS_ADDRESS = 0x0802;
    i2c_inst_t* i2c;
    uint sdaPin;
    uint sclPin;
};



#endif //EEPROM_H
