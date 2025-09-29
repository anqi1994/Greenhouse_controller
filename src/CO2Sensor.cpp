#include "CO2Sensor.h"
#include <cstring>
#include <utility>

CO2Sensor::CO2Sensor(std::shared_ptr<ModbusClient> client, int server_address)
    :
    //register_address 256.
    CO2_read_Register(client, server_address,256,true){}


uint16_t CO2Sensor::read_value(){
    read_CO2_value = CO2_read_Register.read();
    return read_CO2_value;
}

// GMP252 input registers (Function 04):
// IR 30257 -> wire 256: 16-bit signed integer, ppm (0..~32000)
// IR 30258 -> wire 257: 16-bit signed integer, scaled (raw/10 -> ppm, 0..~320000)

/*static constexpr int WIRE_CO2_INT    = 256; // 30257 - 30001
static constexpr int WIRE_CO2_SCALED = 257; // 30258 - 30001

CO2Sensor::CO2Sensor(std::shared_ptr<ModbusClient> client,
                     int slave,
                     bool use_scaled)
    : client_(std::move(client)),
      slave_(slave),
      scaled_(use_scaled),
      reg_(client_, slave_, use_scaled ? WIRE_CO2_SCALED : WIRE_CO2_INT)
{
}

void CO2Sensor::setUseScaled(bool use_scaled)
{
    scaled_ = use_scaled;
    // Rebind the register to the chosen wire address
    reg_ = ModbusRegister(client_, slave_,
                          scaled_ ? WIRE_CO2_SCALED : WIRE_CO2_INT);
}

int CO2Sensor::readPpm()
{
    // Single read attempt
    int raw = reg_.read();

    if (raw < 0) {
        // Keep previous value on failure
        last_ok_ = false;
        return last_ppm_;
    }

    last_raw_ = raw;

    // Convert raw to ppm according to selected register
    int ppm = scaled_ ? (raw / 10) : raw;

    // Basic sanity clamp
    if (ppm < 0)      ppm = 0;
    if (ppm > 15000)  ppm = 15000;

    last_ppm_ = ppm;
    last_ok_  = true;
    return last_ppm_;
}*/

