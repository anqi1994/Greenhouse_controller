#include <iostream>
#include <sstream>
#include <pico/stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"
#include "hardware/gpio.h"
#include "PicoOsUart.h"
#include "ssd1306.h"
#include "modbus/ModbusClient.h"
#include "Structs.h"
#include <event_groups.h>

#include "Task_Network/Network.h"
#include "Task_Control/Control.h"
#include "Task_UI/UI.h"

#include "hardware/timer.h"
extern "C" {
uint32_t read_runtime_ctr(void) {
    return timer_hw->timerawl;
}
}

TimerHandle_t measure_timer;
SemaphoreHandle_t measure_semaphore;

void timer_callback(TimerHandle_t xTimer) {
    xSemaphoreGive(measure_semaphore);
}

int main() {
    stdio_init_all();

    EventGroupHandle_t network_event_group = xEventGroupCreate();

    // timer and semaphore used to measure and send data at fixed intervals
    measure_timer = xTimerCreate("measure_timer", pdMS_TO_TICKS(20000), pdTRUE, nullptr, timer_callback);
    measure_semaphore = xSemaphoreCreateBinary();

    QueueHandle_t to_control = xQueueCreate(10, sizeof(Message));
    QueueHandle_t to_UI = xQueueCreate(10, sizeof(Message));
    QueueHandle_t to_network = xQueueCreate(10, sizeof(Message));

    xTimerStart(measure_timer, 0);

    Control control_task(measure_semaphore, to_UI,to_network,to_control,network_event_group);
    UI ui_task(to_control,to_network,to_UI,network_event_group);
    Network network_task(to_control,to_UI,to_network,network_event_group);

    vTaskStartScheduler();

    while(true) {};
}
