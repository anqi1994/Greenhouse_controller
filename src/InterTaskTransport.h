#ifndef INTER_TASK_TRANSPORT_H
#define INTER_TASK_TRANSPORT_H
// InterTaskTransport: centralized inter-task data transport for Control, UI, and Network.
// C++ interface with .h header as requested. Uses your original Structs.h verbatim.

#include <cstdint>
#include <cstddef>

#include "FreeRTOS.h"
#include "queue.h"

#include "pico/types.h"   // for 'uint' to match your Structs.h
#include "Structs.h"      // uses your Monitored_data, MessageType, Message

class InterTaskTransport {
public:
    // 1) Lifecycle
    InterTaskTransport() noexcept;
    ~InterTaskTransport();

    // Non-copyable to avoid double-free; movable if needed.
    InterTaskTransport(const InterTaskTransport&)            = delete;
    InterTaskTransport& operator=(const InterTaskTransport&) = delete;
    InterTaskTransport(InterTaskTransport&&)                 = default;
    InterTaskTransport& operator=(InterTaskTransport&&)      = default;

    // Initialize all queues; depths are item counts (not bytes).
    bool init(size_t depth_to_control,
              size_t depth_to_UI,
              size_t depth_to_network);

    // 2) UI-facing
    // Block to receive a Message destined for the UI (from Control or Network).
    bool uiReceive(Message& out_msg, uint32_t timeout_ms);

    // Forward a CO2 setpoint to Network (Message) and Control (raw uint).
    bool uiForwardSetpoint(uint co2_set_ppm, uint32_t timeout_ms);

    // 3) Control-facing
    // Control publishes monitored data to UI.
    bool controlPublishMonitoredData(uint16_t co2_ppm,
                                     double temperature,
                                     double humidity,
                                     double pressure,
                                     uint32_t timeout_ms);

    // Control receives a raw setpoint (uint) from UI.
    bool controlReceiveSetpoint(uint& out_set_ppm, uint32_t timeout_ms);

    // 4) Network-facing
    // Network publishes a setpoint (Message) to UI.
    bool networkPublishSetpoint(uint co2_set_ppm, uint32_t timeout_ms);

    // 5) Optional: expose queue handles if needed elsewhere
    QueueHandle_t queueToControl()  const noexcept { return to_control_;  }
    QueueHandle_t queueToUI()       const noexcept { return to_UI_;       }
    QueueHandle_t queueToNetwork()  const noexcept { return to_network_;  }

private:
    static TickType_t msToTicks(uint32_t ms) noexcept {
        return pdMS_TO_TICKS(ms);
    }

    void reset_queues() noexcept;

private:
    QueueHandle_t to_control_  = nullptr;  // expects 'uint'
    QueueHandle_t to_UI_       = nullptr;  // expects 'Message'
    QueueHandle_t to_network_  = nullptr;  // expects 'Message'
};

#endif // INTER_TASK_TRANSPORT_H
