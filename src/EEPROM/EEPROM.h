#ifndef EEPROM_H
#define EEPROM_H

#include "pico/stdlib.h"
#include "PicoI2C.h"
#include <cstring>
#include <string>
#include <memory>

#define EEPROM_ADDRESS 0x50
#define STATUS_BUFF_SIZE 8 // for status updates
#define STR_BUFFER_SIZE 64 //for log messages

#define STATUS_MSG_COUNT 5 // 5 addresses saved for status updates like co2_set val or reboot detect
//addresses for specific status updates
#define REBOOT_ADDR 0x00
#define CO2_SET_ADDR (REBOOT_ADDR + STATUS_BUFF_SIZE)
#define FAN_SPEED_ADDR (CO2_SET_ADDR + STATUS_BUFF_SIZE)

#define LOG_COUNT 10 // max log messages until log entries are deleted

//address for saving wifi credentials
#define WIFI_SSID_ADDR (STATUS_MSG_COUNT * STATUS_BUFF_SIZE)
#define WIFI_PASS_ADDR (WIFI_SSID_ADDR + STR_BUFFER_SIZE)

#define MIN_LOG_ADDR (WIFI_PASS_ADDR + STR_BUFFER_SIZE)
#define MAX_LOG_ADDRESS (MIN_LOG_ADDR + (LOG_COUNT - 1) * STR_BUFFER_SIZE)
#define LOG_ADDR_STORAGE (MAX_LOG_ADDRESS + STR_BUFFER_SIZE)

class EEPROM {
public:
    EEPROM(std::shared_ptr<PicoI2C> i2cbus, uint8_t address = EEPROM_ADDRESS);

    // single status updates
    bool writeStatus(uint16_t address, const char *status);
    bool readStatus(uint16_t address, char *status_buffer, size_t buffer_len);
    bool readStatus(uint16_t address, std::string &status_buffer);

    // functions for logging
    bool writeLog(const char *message);
    void printAllLogs();
    void deleteLogs();
    bool isLogEmpty(uint16_t *next_addr);

    // direct access functions
    bool eepromWrite(uint16_t address, const uint8_t *data, size_t data_len);
    bool eepromRead(uint16_t address, uint8_t *data, size_t data_len);

private:
    std::shared_ptr<PicoI2C> i2c;
    uint8_t addr;

    // private functions for crc check and log address update
    uint16_t crc16(const uint8_t *buffer_p, size_t buffer_len);
    bool validateCrc(const uint8_t *data_buffer, size_t message_len);
    uint16_t readLogAddress();
    bool writeLogAddress(uint16_t log_addr);
};

#endif // EEPROM_H