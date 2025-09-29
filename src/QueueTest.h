#ifndef QUEUETEST_H
#define QUEUETEST_H
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"


class QueueTest {
    public:
        QueueTest(QueueHandle_t from_Control, uint32_t stack_size = 512, UBaseType_t priority = tskIDLE_PRIORITY + 1);
        static void task_wrap(void *pvParameters);
    private:
        void task_impl();
        QueueHandle_t from_Control;
        const char *name = "TEST";

};



#endif //QUEUETEST_H
