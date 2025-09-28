#ifndef CO2SENSOR_H
#define CO2SENSOR_H

#include "modbus/ModbusRegister.h"

class CO2Sensor {
public:
    CO2Sensor(std::shared_ptr<ModbusClient> client, int co2_server_address, int temp_rh_server_address);

    int readCO2();
    int readTemperature();
    int readHumidity();
    
    // Get last read values without reading from Modbus
    int getCO2() const;
    int getTemperature() const;
    int getHumidity() const;
    
    // Read all sensors at once
    void readAllSensors();
    
    // Check if CO2 level is above limit
    bool isCO2AboveLimit(int limit) const;
    bool isCO2AboveSetpoint(int setpoint, int offset = 0) const;
    bool isCO2BelowSetpoint(int setpoint, int offset = 0) const;

private:
    ModbusRegister co2_register;
    ModbusRegister rh_register;
    ModbusRegister temp_register;
    
    int current_co2 = 0;
    int current_temperature = 0;
    int current_humidity = 0;
};

#endif //CO2SENSOR_H
