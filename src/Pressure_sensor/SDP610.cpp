#include "SDP610.h"

SDP610::SDP610() {
    i2cbus = std::make_shared<PicoI2C>(1, 100000);

    vTaskDelay(pdMS_TO_TICKS(100));

    printf("SDP610 initialized\n");
}

uint SDP610::read() {
    uint8_t buffer[3];
    auto rv = i2cbus->read(addr, buffer, 3);

    if (rv == 3) {
        int16_t raw_val = buffer[0] << 8 | buffer[1];
        return raw_val;
    }
    return 0;
}

void SDP610::printHex(int16_t value) {
    printf("Raw hex: 0x%04X\n", (uint16_t)value);
    printf("Signed decimal: %d\n", value);
}

void SDP610::scanI2C() {
    bool found = false;

    for (uint8_t test_addr = 0x08; test_addr <= 0x77; ++test_addr) {
        uint8_t dummy;
        auto result = i2cbus->read(test_addr, &dummy, 1);
        if (result == 1) {
            printf("Found device at address: 0x%02X\n", test_addr);
            found = true;
        }
    }

    if (!found) {
        printf("No I2C devices found\n");
    }
}