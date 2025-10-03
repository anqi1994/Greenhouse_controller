#include "QueueTest.h"

#include <cstdio>

#include "Structs.h"

QueueTest::QueueTest(QueueHandle_t to_CO2, QueueHandle_t to_Network, QueueHandle_t to_UI,uint32_t stack_size, UBaseType_t priority):
    to_CO2(to_CO2),to_Network(to_Network) ,to_UI (to_UI){

    xTaskCreate(task_wrap, name, stack_size, this, priority, nullptr);
}

void QueueTest::task_wrap(void *pvParameters) {
    auto *test = static_cast<QueueTest*>(pvParameters);
    test->task_impl();
}

void QueueTest::task_impl() {
    Message received{};

    Message send{};

    //this is just for testing
    uint co2_set = 1800;
    bool tested = true;

    while (true) {
        //get data from CO2_control and network. data type received: 1. monitored data. 2. uint CO2 set level.
        if (xQueueReceive(to_UI, &received, portMAX_DELAY)) {
            //the received data is from CO2_control_task
            if(received.type == MONITORED_DATA){
                //printf("QUEUE to UI from CO2_control_task: co2: %d\n", received.data.co2_val);
                //printf("QUEUE to UI from CO2_control_task: temp: %.1f\n", received.data.temperature);
            }
            //the received data is from network task
            else if(received.type == CO2_SET_DATA){
                co2_set = received.co2_set;
                //printf("QUEUE to UI from network: co2_set: %d\n", co2_set);
            }

        }

        //sending CO2 data, needs to be triggered when the user change
        //now it's test
        if(!tested){
            //queue to network, it needs to send type, co2 set level.
            send.type = CO2_SET_DATA;
            send.co2_set = co2_set;
            xQueueSendToBack(to_Network, &send, portMAX_DELAY);
            printf("QUEUE from UI from network: %d\n", send.co2_set);
            //queue to co2, it only needs to send uint co2 set value.
            xQueueSendToBack(to_CO2, &co2_set, portMAX_DELAY);
            printf("QUEUE from UI to CO2: %d\n", co2_set);
            tested = true;
        }
    }
}

