
#ifndef PICO_MODBUS_EEPROM_H
#define PICO_MODBUS_EEPROM_H

#include <cstdio>
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "PicoI2C.h"
#include "memory"

#define I2C0_SDA_PIN        16
#define I2C0_SCL_PIN        17
#define I2C_0               0
#define EEPROM_ADDR         0x50
#define I2C_BAUD            100000
#define WRITE_CYCLE_TIME    5
#define ONE_BYTE            8
#define MEM_ADDR_BYTES      2
#define MASK_0XFF           0xFF
#define MAX_DATA_BUFF       64

#define SSID_ADDR           (0)
#define PASS_ADDR           (64)
#define CO2_SETPOINT_ADDR   (128)

extern uint16_t setpoint;
extern char SSID_WIFI[MAX_DATA_BUFF];
extern char PASS_WIFI[MAX_DATA_BUFF];
bool writeEEPROM(const std::shared_ptr<PicoI2C>& i2c, uint16_t memAddr, const uint8_t *data, size_t length);
bool readEEPROM(const std::shared_ptr<PicoI2C>& i2c, uint16_t memAddr, uint8_t *data, size_t length);
void readAtBoot(const std::shared_ptr<PicoI2C>& eeprom);

#endif //PICO_MODBUS_EEPROM_H