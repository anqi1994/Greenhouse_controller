#include "GMP252.h"
#include <cstring>
#include <utility>

GMP252::GMP252(std::shared_ptr<ModbusClient> client, int server_address)
    :
    //register_address 256.
    CO2_read_Register(client, server_address,256,true){}


uint16_t GMP252::read_value(){
    read_CO2_value = CO2_read_Register.read();
    return read_CO2_value;
}
