#include "HMP60.h"

HMP60::HMP60(std::shared_ptr<ModbusClient> client, int server_address):
    rh_register(client, server_address, 256,true), //RH register number 257, need to -1
    temp_register(client, server_address, 257,true) //Temperature register number 258, need to -1
{}

double HMP60::read_tem(){
    temperature = ( temp_register.read()/10.0);
    return temperature;
}

double HMP60::read_hum(){
    humidity = ( rh_register.read()/10.0);
    return humidity;
}