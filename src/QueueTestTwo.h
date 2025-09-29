#ifndef QUEUETESTTWO_H
#define QUEUETESTTWO_H
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"


class QueueTestTwo {
public:
    QueueTestTwo(QueueHandle_t to_CO2, QueueHandle_t to_UI, QueueHandle_t to_Network, uint32_t stack_size = 512, UBaseType_t priority = tskIDLE_PRIORITY + 1);
    static void task_wrap(void *pvParameters);
private:
    void task_impl();
    QueueHandle_t to_CO2;
    QueueHandle_t to_UI;
    QueueHandle_t to_Network;
    const char *name = "TESTTWO";

};



#endif //QUEUETEST_H
