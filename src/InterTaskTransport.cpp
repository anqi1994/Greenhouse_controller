#include "InterTaskTransport.h"
#include <cstdio>
#include <cstring>

// 1) Lifecycle
InterTaskTransport::InterTaskTransport() noexcept = default;

InterTaskTransport::~InterTaskTransport() {
    reset_queues();
}

void InterTaskTransport::reset_queues() noexcept {
    if (to_UI_)      { vQueueDelete(to_UI_);      to_UI_ = nullptr; }
    if (to_network_) { vQueueDelete(to_network_); to_network_ = nullptr; }
    if (to_control_) { vQueueDelete(to_control_); to_control_ = nullptr; }
}

bool InterTaskTransport::init(size_t depth_to_control,
                              size_t depth_to_UI,
                              size_t depth_to_network)
{
    // Clean up old queues if re-init.
    reset_queues();

    to_UI_      = xQueueCreate(static_cast<UBaseType_t>(depth_to_UI),      sizeof(Message));
    to_network_ = xQueueCreate(static_cast<UBaseType_t>(depth_to_network), sizeof(Message));
    to_control_ = xQueueCreate(static_cast<UBaseType_t>(depth_to_control), sizeof(uint));

    if (!to_UI_ || !to_network_ || !to_control_) {
        reset_queues();
        return false;
    }
    return true;
}

// 2) UI-facing
bool InterTaskTransport::uiReceive(Message& out_msg, uint32_t timeout_ms) {
    if (!to_UI_) return false;
    return xQueueReceive(to_UI_, &out_msg, msToTicks(timeout_ms)) == pdPASS;
}

bool InterTaskTransport::uiForwardSetpoint(uint co2_set_ppm, uint32_t timeout_ms) {
    if (!to_network_ || !to_control_) return false;

    // 2.1 Send to Network as Message
    Message m{};
    m.type     = CO2_SET_DATA;  // use your original enum value
    m.co2_set  = co2_set_ppm;

    bool ok_net  = xQueueSend(to_network_, &m, msToTicks(timeout_ms)) == pdPASS;
    if (!ok_net) {
        std::printf("UI->NETWORK: queue full, drop setpoint %u\n", static_cast<unsigned>(co2_set_ppm));
    } else {
        std::printf("UI->NETWORK: setpoint %u sent\n", static_cast<unsigned>(co2_set_ppm));
    }

    // 2.2 Send to Control as raw uint
    bool ok_ctrl = xQueueSend(to_control_, &co2_set_ppm, msToTicks(timeout_ms)) == pdPASS;
    if (!ok_ctrl) {
        std::printf("UI->CONTROL: queue full, drop setpoint %u\n", static_cast<unsigned>(co2_set_ppm));
    } else {
        std::printf("UI->CONTROL: setpoint %u sent\n", static_cast<unsigned>(co2_set_ppm));
    }

    return ok_net && ok_ctrl;
}

// 3) Control-facing
bool InterTaskTransport::controlPublishMonitoredData(uint16_t co2_ppm,
                                                     double temperature,
                                                     double humidity,
                                                     double pressure,
                                                     uint32_t timeout_ms)
{
    if (!to_UI_) return false;

    Message m{};
    m.type = MONITORED_DATA;
    m.data.co2_val   = co2_ppm;
    m.data.temperature = temperature;
    m.data.humidity    = humidity;
    m.data.pressure    = pressure;

    return xQueueSend(to_UI_, &m, msToTicks(timeout_ms)) == pdPASS;
}

bool InterTaskTransport::controlReceiveSetpoint(uint& out_set_ppm, uint32_t timeout_ms) {
    if (!to_control_) return false;
    return xQueueReceive(to_control_, &out_set_ppm, msToTicks(timeout_ms)) == pdPASS;
}

// 4) Network-facing
bool InterTaskTransport::networkPublishSetpoint(uint co2_set_ppm, uint32_t timeout_ms) {
    if (!to_UI_) return false;

    Message m{};
    m.type    = CO2_SET_DATA;
    m.co2_set = co2_set_ppm;

    return xQueueSend(to_UI_, &m, msToTicks(timeout_ms)) == pdPASS;
}
