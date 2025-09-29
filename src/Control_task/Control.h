#ifndef CONTROL_H
#define CONTROL_H

#define UART_NR 1
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#define BAUD_RATE 9600
#define STOP_BITS 2

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "Fan.h"
#include "CO2Sensor.h"
#include "TemHumSensor.h"
#include "Valve.h"
#include "Structs.h"


class Control {
public:
    Control(SemaphoreHandle_t timer, uint32_t stack_size = 512, UBaseType_t priority = tskIDLE_PRIORITY + 1);
    static void task_wrap(void *pvParameters);


private:
    // Private functions
    void task_impl();
    bool check_fan(Fan &fan);

    SemaphoreHandle_t timer_semphr;
    TaskHandle_t control_task;
    const char *name = "CONTROL";
    uint max_co2 = 2000;
    bool fan_working = true;
};

#endif //CONTROL_H
