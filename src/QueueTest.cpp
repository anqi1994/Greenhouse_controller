#include "QueueTest.h"

#include <cstdio>

#include "Structs.h"

QueueTest::QueueTest(QueueHandle_t from_Control, uint32_t stack_size, UBaseType_t priority):
    from_Control(from_Control) {

    xTaskCreate(task_wrap, name, stack_size, this, priority, nullptr);
}

void QueueTest::task_wrap(void *pvParameters) {
    auto *test = static_cast<QueueTest*>(pvParameters);
    test->task_impl();
}

void QueueTest::task_impl() {
    Monitored_data received;

    while (true) {
        if (xQueueReceive(from_Control, &received, portMAX_DELAY)) {
            printf("QUEUE co2: %d\n", received.co2_val);
            printf("QUEUE temp: %.1f\n", received.temperature);
        }
    }
}

