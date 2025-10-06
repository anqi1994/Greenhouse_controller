#include "SDP610.h"

SDP610::SDP610(std::shared_ptr<PicoI2C> i2cbus, uint8_t address):
    i2c(std::move(i2cbus)), addr(address) {

    uint8_t reset_cmd = 0x06;
    i2c->write(addr, &reset_cmd, 1);
    vTaskDelay(pdMS_TO_TICKS(20));

    //printf("SDP610 initialized at address 0x%02X\n", addr);

}

int16_t SDP610::read_raw() {
    // command to take measurements from the SDP610 – 125Pa datasheet
    uint8_t command = 0xF1;

    auto write = i2c->write(addr, &command, 1);
    if (write != 1) {
        printf("SDP610 instruction failed\n");
        return 0;
    }
    vTaskDelay(pdMS_TO_TICKS(10));

    // read and return raw value (two bytes in the buffer)
    uint8_t buffer[3];
    auto rv = i2c->read(addr, buffer, 3);

    if (rv == 3) {
        int16_t raw_val = (buffer[0] << 8) | buffer[1];
        return raw_val;
    }
    printf("Failed to read pressure (got %d bytes)\n", rv);
    return 0;
}
// returing scaled value
double SDP610::read() {
    int16_t raw_val = read_raw();
    // scale factor for SDP610 – 125Pa: 240 (from datasheet)
    return raw_val/240.0f;
}