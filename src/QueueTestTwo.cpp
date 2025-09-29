#include "QueueTestTwo.h"

#include <cstdio>

#include "Structs.h"

QueueTestTwo::QueueTestTwo(QueueHandle_t to_CO2,  QueueHandle_t to_UI, QueueHandle_t to_Network,uint32_t stack_size, UBaseType_t priority):
    to_CO2(to_CO2),to_UI (to_UI),to_Network(to_Network){

    xTaskCreate(task_wrap, name, stack_size, this, priority, nullptr);
}

void QueueTestTwo::task_wrap(void *pvParameters) {
    auto *test = static_cast<QueueTestTwo*>(pvParameters);
    test->task_impl();
}

void QueueTestTwo::task_impl() {
    Message received{};
    Message send{};

    //test data
    uint co2_set = 1400;
    bool tested = false;

    while (true) {
        //get data from CO2_control and UI. data type received: 1. monitored data. 2. uint CO2 set level.
        if (xQueueReceive(to_Network, &received, portMAX_DELAY)) {
            //the received data is from CO2_control_task
            if(received.type == MONITORED_DATA){
                printf("QUEUE to Network from CO2_control_task: co2: %d\n", received.data.co2_val);
                printf("QUEUE to Network from CO2_control_task: temp: %.1f\n", received.data.temperature);
            }
        }
        //the received data is from network task
        else if(received.type == CO2_SET_DATA){
            co2_set = received.co2_set;
            printf("QUEUE to network from UI: co2_set: %d\n", co2_set);
        }

        if(!tested){
            //queue to UI, it needs to send type, co2 set level.
            send.type = CO2_SET_DATA;
            send.co2_set = co2_set;
            xQueueSendToBack(to_UI, &co2_set, portMAX_DELAY);
            printf("QUEUE from network from UI: co2_set: %d\n", co2_set);
            //queue to co2, it only needs to send uint co2 set value.
            xQueueSendToBack(to_CO2, &co2_set, portMAX_DELAY);
            printf("QUEUE from network to co2: co2_set: %d\n", co2_set);
            tested = true;
        }
    }
}

