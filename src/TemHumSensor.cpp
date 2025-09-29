#include "TemHumSensor.h"

TemHumSensor::TemHumSensor(std::shared_ptr<ModbusClient> client, int server_address):
    rh_register(client, server_address, 256,true), //RH register number 257, need to -1
    temp_register(client, server_address, 257,true) //Temperature register number 258, need to -1
{}

uint16_t TemHumSensor::read_tem(){
    temperature = ( temp_register.read()/10);
    return temperature;
}

uint16_t TemHumSensor::read_hum(){
    humidity = ( rh_register.read()/10);
    return humidity;
}