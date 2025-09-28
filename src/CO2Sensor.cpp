#include "CO2Sensor.h"

// Constructor using the same addresses from your original code
CO2Sensor::CO2Sensor(std::shared_ptr<ModbusClient> client, int co2_server_address, int temp_rh_server_address)
    : co2_register(client, co2_server_address, 256),      // Server 240, register 256
      rh_register(client, temp_rh_server_address, 256),   // Server 241, register 256
      temp_register(client, temp_rh_server_address, 257)  // Server 241, register 257
{}

// Read CO2 value from Modbus
int CO2Sensor::readCO2() {
    current_co2 = co2_register.read();
    return current_co2;
}

// Read temperature value from Modbus (divide by 10 as in original code)
int CO2Sensor::readTemperature() {
    current_temperature = temp_register.read() / 10;
    return current_temperature;
}

// Read humidity value from Modbus (divide by 10 as in original code)
int CO2Sensor::readHumidity() {
    current_humidity = rh_register.read() / 10;
    return current_humidity;
}

// Get last read CO2 value without reading from Modbus
int CO2Sensor::getCO2() const {
    return current_co2;
}

// Get last read temperature value without reading from Modbus
int CO2Sensor::getTemperature() const {
    return current_temperature;
}

// Get last read humidity value without reading from Modbus
int CO2Sensor::getHumidity() const {
    return current_humidity;
}

// Read all sensors at once with delays as in original code
void CO2Sensor::readAllSensors() {
    current_co2 = co2_register.read();
    vTaskDelay(pdMS_TO_TICKS(5));
    
    current_humidity = rh_register.read() / 10;
    vTaskDelay(pdMS_TO_TICKS(5));
    
    current_temperature = temp_register.read() / 10;
    vTaskDelay(pdMS_TO_TICKS(5));
}

// Check if CO2 level is above a specified limit
bool CO2Sensor::isCO2AboveLimit(int limit) const {
    return current_co2 > limit;
}

// Check if CO2 level is above setpoint (with optional offset)
bool CO2Sensor::isCO2AboveSetpoint(int setpoint, int offset) const {
    return (current_co2 - offset) > setpoint;
}

// Check if CO2 level is below setpoint (with optional offset)
bool CO2Sensor::isCO2BelowSetpoint(int setpoint, int offset) const {
    return (current_co2 + offset) < setpoint;
}
