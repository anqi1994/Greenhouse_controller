#include "QueueTest.h"

#include <cstdio>

#include "Structs.h"

QueueTest* QueueTest::instance = nullptr;

QueueTest::QueueTest(QueueHandle_t to_CO2, QueueHandle_t to_Network, QueueHandle_t to_UI,uint32_t stack_size, UBaseType_t priority):
    to_CO2(to_CO2),
    to_Network(to_Network) ,
    to_UI(to_UI),
    rotA(10, true, false, false),
    rotB(11),
    sw(12) {

    //to be used in the isr accessing
    instance = this;

    encoder_queue = xQueueCreate(30, sizeof(encoderEv));
    vQueueAddToRegistry(encoder_queue, "Encoder_Q");

    gpio_set_irq_enabled_with_callback(rotA, GPIO_IRQ_EDGE_FALL, true, &encoder_callback);
    gpio_set_irq_enabled(sw, GPIO_IRQ_EDGE_FALL, true);

    // creating the task
    xTaskCreate(task_wrap, name, stack_size, this, priority, nullptr);
}

void QueueTest::encoder_callback(uint gpio, uint32_t events) {
    if (instance) {
        instance->handleEncoderCallback(gpio, events);
    }
}
// implementation of detectin encoder events
void QueueTest::handleEncoderCallback(uint gpio, uint32_t events) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    encoderEv ev;
    TickType_t current_time = xTaskGetTickCountFromISR();

    if (gpio == rotA) {
        ev = (rotB.read() == 0) ? ROT_L : ROT_R;
        xQueueSendToBackFromISR(encoder_queue, &ev, &xHigherPriorityTaskWoken);
    }

    if (gpio == sw) {
        if ((current_time - last_sw_time) >= pdMS_TO_TICKS(250)) {
            ev = SW;
            xQueueSendToBackFromISR(encoder_queue, &ev, &xHigherPriorityTaskWoken);
            last_sw_time = current_time;
        }
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void QueueTest::task_wrap(void *pvParameters) {
    auto *test = static_cast<QueueTest*>(pvParameters);
    test->task_impl();
}

void QueueTest::task_impl() {
    Message received{};
    Message send{};
    encoderEv ev;

    auto i2cbus1 = std::make_shared<PicoI2C>(1, 4000000);
    ssd1306os display(i2cbus1);
    display.fill(0);
    display.text("Hello", 0, 0);
    display.show();

    //this is just for testing
    uint co2_set = 1800;
    bool tested = true;

    while (true) {
        // checking encoder queue without PortMAX to not block if there are no events
        if (xQueueReceive(encoder_queue, &ev, pdMS_TO_TICKS(10))) {
            switch (ev) {
                case ROT_R:
                    printf("%d\n", ev);
                    //display.fill(0);
                    //display.text("1", 0, 0);
                    //display.show();
                break;
                case ROT_L:
                    printf("%d\n", ev);
                    //display.fill(0);
                    //display.text("-1", 0, 0);
                    //display.show();
                break;
                case SW:
                    printf("%d\n", ev);
                    //display.fill(0);
                    //display.text("0", 0, 0);
                    //display.show();
                break;
            }
        }

        //get data from CO2_control and network. data type received: 1. monitored data. 2. uint CO2 set level.
        if (xQueueReceive(to_UI, &received, pdMS_TO_TICKS(10))) {
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

