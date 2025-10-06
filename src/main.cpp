// main_ui_test.cpp
// Minimal main to test ONLY the UI task (no SystemContext, no Control/Network).
// Queues carry 'Message' as defined in structs.h.

#include <cstdio>
#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

#include "structs.h"   // Message, Monitored_data, MessageType
#include "UI-task/UI.h"        // Your UI task (integrated encoder ISR)

// ---------------- Build-time toggles ----------------
#ifndef ENABLE_UI_FEEDER
#define ENABLE_UI_FEEDER 1   // Set to 0 to disable the dummy feeder task
#endif

// ---------------- Stack sizes & priorities (tune as needed) ----------------
static constexpr uint32_t UI_STACK   = 2048;                // bytes
static constexpr UBaseType_t UI_PRIO = tskIDLE_PRIORITY+1;

static constexpr UBaseType_t Q_LEN   = 16;                  // queue depth

#if ENABLE_UI_FEEDER
// Tiny feeder task: periodically send fake monitored data and setpoint updates to UI.
static void ui_feeder_task(void* arg) {
    QueueHandle_t to_UI = static_cast<QueueHandle_t>(arg);
    uint32_t tick = 0;

    for (;;) {
        // 1) Send monitored data (Control -> UI)
        Message m{};
        m.type = MONITORED_DATA;
        m.data.co2_val    = static_cast<uint16_t>(600 + (tick % 40) * 10); // 600..1000 ppm
        m.data.temperature = 22.5;
        m.data.humidity    = 40.0;
        m.data.fan_speed   = (tick % 2) ? 35 : 20;
        xQueueSendToBack(to_UI, &m, portMAX_DELAY);

        // 2) Every ~5 s, send a setpoint update as if from Network -> UI
        /*
        if ((tick % 5) == 4) {
            Message s{};
            s.type    = CO2_SET_DATA;
            s.co2_set = 1200 + ((tick / 5) % 4) * 100; // 1200,1300,1400,1500...
            xQueueSendToBack(to_UI, &s, portMAX_DELAY);
        }
        */

        tick++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
#endif

int main() {
    stdio_init_all();

    // 1) Create three queues (all carry 'Message')
    QueueHandle_t to_UI      = xQueueCreate(Q_LEN, sizeof(Message));
    QueueHandle_t to_Network = xQueueCreate(Q_LEN, sizeof(Message));
    QueueHandle_t to_CO2     = xQueueCreate(Q_LEN, sizeof(Message));
    configASSERT(to_UI && to_Network && to_CO2);

    // 2) Construct UI task (registers encoder IRQs internally)
    static UI ui(to_CO2, to_Network, to_UI, UI_STACK, UI_PRIO);

#if ENABLE_UI_FEEDER
    // 3) Start dummy feeder so the UI has data without Control/Network
    xTaskCreate(ui_feeder_task, "ui_feed", 1024, to_UI, tskIDLE_PRIORITY+1, nullptr);
#endif

    // 4) Start scheduler
    vTaskStartScheduler();

    // Should never reach here
    while (true) { tight_loop_contents(); }
    return 0;
}
