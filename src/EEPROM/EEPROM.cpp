#include "EEPROM.h"

#include <array>
#include <vector>
#include <bits/fs_fwd.h>

EEPROM::EEPROM(std::shared_ptr<PicoI2C> i2cbus, uint8_t address):
    i2c(std::move(i2cbus)), addr(address) {}

uint16_t EEPROM::crc16(const uint8_t *buffer_p, size_t buffer_len) {
    uint8_t x;
    uint16_t crc = 0xFFFF;

    while (buffer_len--) {
        x = (crc >> 8) ^ *buffer_p++;
        x ^= x >> 4;
        crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x << 5)) ^ ((uint16_t)x);
    }
    return crc;
}

bool EEPROM::validateCrc(const uint8_t *data_buffer, size_t message_len) {
    uint16_t stored_crc = (static_cast<uint16_t>(data_buffer[message_len + 1]) << 8) |
                          static_cast<uint16_t>(data_buffer[message_len + 2]);
    uint16_t calculated_crc = crc16(data_buffer, message_len + 1);
    return calculated_crc == stored_crc;
}

// basic write
bool EEPROM::eepromWrite(uint16_t address, const uint8_t *data, size_t data_len) {
    std::array<uint8_t, 2> addr_buf = {
        static_cast<uint8_t>((address >> 8) & 0xFF),
        static_cast<uint8_t>(address & 0xFF)
    };

    std::vector<uint8_t> data_buf(data_len + 2);
    std::memcpy(data_buf.data(), addr_buf.data(), addr_buf.size());
    std::memcpy(data_buf.data() + 2, data, data_len);

    uint result = i2c->write(addr, data_buf.data(), data_len + 2);
    // 5ms for writing
    vTaskDelay(pdMS_TO_TICKS(5));
    return result == (data_len + 2);
}

bool EEPROM::eepromRead(uint16_t address, uint8_t *data, size_t data_len) {
    std::array<uint8_t, 2> addr_buf = {
        static_cast<uint8_t>((address >> 8) & 0xFF),
        static_cast<uint8_t>(address & 0xFF)
    };

    // write address, then reading
    uint result = i2c->transaction(addr, addr_buf.data(), addr_buf.size(), data, data_len);
    return result == (addr_buf.size() + data_len);
}

// writing status to specific address
bool EEPROM::writeStatus(const uint16_t address, const char *status) {
    size_t len = strlen(status);

    if (len > BUFFER_SIZE - 3) {
        len = BUFFER_SIZE - 3;
    }

    std::vector<uint8_t> log_message_buff(BUFFER_SIZE, 0);
    std::memcpy(log_message_buff.data(), status, len);
    log_message_buff[len] = '\0';

    uint16_t crc = crc16(log_message_buff.data(), len + 1);
    log_message_buff[len + 1] = static_cast<uint8_t>(crc >> 8);
    log_message_buff[len + 2] = static_cast<uint8_t>(crc & 0xFF);

    return eepromWrite(address, log_message_buff.data(), BUFFER_SIZE);
}

bool EEPROM::readStatus(uint16_t address, char *status_buffer, size_t buffer_len) {
    std::vector<uint8_t> read_buffer(BUFFER_SIZE);

    if (!eepromRead(address, read_buffer.data(), BUFFER_SIZE)) {
        return false;
    }

    size_t message_len = 0;
    while (message_len < BUFFER_SIZE && read_buffer[message_len] != '\0') {
        message_len++;
    }

    if (!validateCrc(read_buffer.data(), message_len)) {
        printf("CRC check failed for status at address 0x%04X\n", address);
        return false;
    }

    size_t copy_len = (message_len < buffer_len - 1) ? message_len : buffer_len - 1;
    std::memcpy(status_buffer, read_buffer.data(), copy_len);
    status_buffer[copy_len] = '\0';

    return true;
}

// overload for std::string
bool EEPROM::readStatus(uint16_t address, std::string &status_buffer) {
    std::vector<uint8_t> read_buffer(BUFFER_SIZE);

    if (!eepromRead(address, read_buffer.data(), BUFFER_SIZE)) {
        return false;
    }

    size_t message_len = 0;
    while (message_len < BUFFER_SIZE && read_buffer[message_len] != '\0') {
        message_len++;
    }

    if (!validateCrc(read_buffer.data(), message_len)) {
        printf("CRC check failed for status at address 0x%04X\n", address);
        return false;
    }

    read_buffer[message_len] = '\0';
    status_buffer = reinterpret_cast<char *>(read_buffer.data());
    return true;
}

// log impl
uint16_t EEPROM::readLogAddress() {
    uint16_t log_addr;
    if (!eepromRead(LOG_ADDR_STORAGE, (uint8_t *)&log_addr, sizeof(log_addr))) {
        return 0;
    }
    return log_addr;
}

bool EEPROM::writeLogAddress(uint16_t log_addr) {
    return eepromWrite(LOG_ADDR_STORAGE, (uint8_t *)&log_addr, sizeof(log_addr));
}

bool EEPROM::isLogEmpty(uint16_t *next_addr) {
    for (uint16_t i = MIN_LOG_ADDR; i <= MAX_LOG_ADDRESS; i += BUFFER_SIZE) {
        uint8_t read_data[BUFFER_SIZE];
        if (eepromRead(i, read_data, sizeof(read_data))) {
            if (read_data[0] == 0) {
                *next_addr = i;
                return true;
            }
        }
    }
    return false;
}

bool EEPROM::writeLog(const char *message) {
    uint16_t log_addr = readLogAddress();

    // check if log full
    if (log_addr == 0xFFFF || log_addr > MAX_LOG_ADDRESS) {
        uint16_t empty_addr;
        if (!isLogEmpty(&empty_addr)) {
            printf("Log full, deleting old logs...\n");
            deleteLogs();
            log_addr = MIN_LOG_ADDR;
        } else {
            log_addr = empty_addr;
        }
    }

    size_t message_len = strlen(message);
    if (message_len > BUFFER_SIZE - 3) {
        message_len = BUFFER_SIZE - 3;
    }

    std::vector<uint8_t> log_message_buf(BUFFER_SIZE, 0);
    std::memcpy(log_message_buf.data(), message, message_len);
    log_message_buf[message_len] = '\0';

    uint16_t crc = crc16(log_message_buf.data(), message_len + 1);
    log_message_buf[message_len + 1] = static_cast<uint8_t>(crc >> 8);
    log_message_buf[message_len + 2] = static_cast<uint8_t>(crc & 0xFF);

    if (!eepromWrite(log_addr, log_message_buf.data(), BUFFER_SIZE)) {
        printf("Failed to write log to EEPROM\n");
        return false;
    }

    log_addr += BUFFER_SIZE;
    if (!writeLogAddress(log_addr)) {
        printf("Failed to update log address\n");
        return false;
    }

    printf("Log written successfully at address 0x%04X\n", log_addr - BUFFER_SIZE);
    return true;
}

void EEPROM::printAllLogs() {
    printf("\n--EEPROM Log--\n");

    for (uint16_t i = MIN_LOG_ADDR; i <= MAX_LOG_ADDRESS; i += BUFFER_SIZE) {
        std::vector<uint8_t> read_data(BUFFER_SIZE);

        if (eepromRead(i, read_data.data(), BUFFER_SIZE)) {
            if (read_data[0] != 0) {
                size_t message_len = 0;
                while (message_len < BUFFER_SIZE && read_data[message_len] != '\0') {
                    message_len++;
                }

                if (validateCrc(read_data.data(), message_len)) {
                    char *message_read = reinterpret_cast<char *>(read_data.data());
                    printf("Log [0x%04X]: %s\n", i, message_read);
                } else {
                    printf("Log [0x%04X]: CRC validation failed\n", i);
                }
            }
        } else {
            printf("Failed to read from address 0x%04X\n", i);
            break;
        }
    }
}

void EEPROM::deleteLogs() {
    uint8_t zero = 0;
    for (uint16_t i = MIN_LOG_ADDR; i <= MAX_LOG_ADDRESS; i += BUFFER_SIZE) {
        eepromWrite(i, &zero, 1);
    }
    writeLogAddress(MIN_LOG_ADDR);
    printf("All logs deleted\n");
}