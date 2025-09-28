//
// Created by lily on 9/19/2025.
//

#include "eeprom.h"
#include "cstring"

// Function to write data to EEPROM at a specific address
bool writeEEPROM(const std::shared_ptr<PicoI2C>& eeprom, uint16_t memAddr, const uint8_t *data, size_t length) {
    // The buffer to hold the address (2 bytes) and the data
    uint8_t buffer[MAX_DATA_BUFF] = {0};

    // Set the memory address (2 bytes)
    buffer[0] = (memAddr >> ONE_BYTE) & MASK_0XFF;  // MSB
    buffer[1] = memAddr & MASK_0XFF;         // LSB

    // Copy the data into the buffer after the memory address
    memcpy(&buffer[2], data, length);

    // Write the buffer (address + data) to the EEPROM
    if (eeprom->write(EEPROM_ADDR, buffer, sizeof(buffer)) == sizeof(buffer)) {
        vTaskDelay(pdMS_TO_TICKS(WRITE_CYCLE_TIME));
        return true;
    }
    return false;
}

// Function to read data from EEPROM at a specific address
bool readEEPROM(const std::shared_ptr<PicoI2C>& eeprom, uint16_t memAddr, uint8_t *data, size_t length) {
    // Send the memory address first (2 bytes) to set the EEPROM read pointer
    uint8_t addrBuffer[MEM_ADDR_BYTES];
    addrBuffer[0] = (memAddr >> ONE_BYTE) & MASK_0XFF;  // MSB
    addrBuffer[1] = memAddr & MASK_0XFF;         // LSB

    // Write the address to the EEPROM without any data, to set the internal pointer
    if (eeprom->write(EEPROM_ADDR, addrBuffer, MEM_ADDR_BYTES) != 2) {
        return false;  // Failed to set address
    }

    // Now read the data starting from the specified address
    if (eeprom->read(EEPROM_ADDR, data, length) != length) {
        return false;  // Failed to read data
    }

    return true;
}

void readAtBoot(const std::shared_ptr<PicoI2C>& eeprom){
    char buff[64] = "\0";
    // Read the ssid from EEPROM
    if (readEEPROM(eeprom, SSID_ADDR, reinterpret_cast<uint8_t *>(buff), MAX_DATA_BUFF)) {
        strcpy(SSID_WIFI, buff);
        printf("SSID: %s\n", SSID_WIFI);
    } else {
        printf("Failed to read SSID.\n");
    }

    // Read the password from EEPROM
    if (readEEPROM(eeprom, PASS_ADDR, reinterpret_cast<uint8_t *>(buff), MAX_DATA_BUFF)) {
        strcpy(PASS_WIFI, buff);
        printf("PASS: %s\n", PASS_WIFI);
    } else {
        printf("Failed to read PASS.\n");
    }

    // Read the Co2 setpoint back from EEPROM
    uint8_t setpointData[2] = {0};
    if (readEEPROM(eeprom, CO2_SETPOINT_ADDR, setpointData, 2)) {
        setpoint = (setpointData[0] << ONE_BYTE) | setpointData[1];
        printf("Co2 setpoint: %u\n", setpoint);
    } else {
        printf("Failed to read setpoint.\n");
    }
}
