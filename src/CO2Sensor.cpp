#include "CO2Sensor.h"
#include <cstring>

// GMP252 input registers (Function 04):
// IR 30257 -> wire 256: 16-bit signed integer, ppm (0..~32000)
// IR 30258 -> wire 257: 16-bit signed integer, scaled (raw/10 -> ppm, 0..~320000)

static constexpr int WIRE_CO2_INT    = 256; // 30257 - 30001
static constexpr int WIRE_CO2_SCALED = 257; // 30258 - 30001

CO2Sensor::CO2Sensor(std::shared_ptr<ModbusClient> client,
                     int slave,
                     bool use_scaled,
                     uint32_t min_interval_ms)
    : client_(std::move(client)),
      slave_(slave),
      scaled_(use_scaled),
      reg_(client_, slave_, use_scaled ? WIRE_CO2_SCALED : WIRE_CO2_INT),
      min_interval_ticks_(pdMS_TO_TICKS(min_interval_ms))
{
}

void CO2Sensor::setUseScaled(bool use_scaled)
{
    scaled_ = use_scaled;
    // Rebind the register to the chosen wire address
    reg_ = ModbusRegister(client_, slave_,
                          scaled_ ? WIRE_CO2_SCALED : WIRE_CO2_INT);
}

int CO2Sensor::readPpm(int retries)
{
    const TickType_t now = xTaskGetTickCount();

    // Rate limiting: return last value if called too frequently
    if (last_read_tick_ != 0 && (now - last_read_tick_) < min_interval_ticks_) {
        return last_ppm_;
    }

    // Perform read with simple retry
    int raw = -1;
    for (int i = 0; i <= retries; ++i) {
        raw = reg_.read();          // Negative means transport/protocol error in your stack
        if (raw >= 0) break;
        vTaskDelay(pdMS_TO_TICKS(50)); // small backoff between retries
    }

    last_read_tick_ = now;

    if (raw < 0) {
        // Keep previous value on failure
        last_ok_ = false;
        return last_ppm_;
    }

    last_raw_ = raw;

    // Convert raw to ppm according to selected register
    int ppm = scaled_ ? (raw / 10) : raw;   // integer division; GMP252 scaled is 0.1 ppm units

    // Basic sanity clamp (adjust bounds as needed by your project)
    if (ppm < 0)      ppm = 0;
    if (ppm > 15000)  ppm = 15000;          // your spec upper limit is 1500 setpoint, but sensor can read higher

    last_ppm_ = ppm;
    last_ok_  = true;
    return last_ppm_;
}
