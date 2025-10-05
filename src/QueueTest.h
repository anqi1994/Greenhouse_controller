#ifndef QUEUETEST_H
#define QUEUETEST_H
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "GPIO/GPIO.h"
#include "ssd1306os.h"

enum encoderEv {
    ROT_L = -1,
    ROT_R = 1,
    SW = 0
};

class QueueTest {
    public:
        QueueTest(QueueHandle_t to_CO2, QueueHandle_t to_Network, QueueHandle_t to_UI,
                uint32_t stack_size = 512, UBaseType_t priority = tskIDLE_PRIORITY + 1);

        static void task_wrap(void *pvParameters);
        // function for encoder irq enabled with callback
        static void encoder_callback(uint gpio, uint32_t events);

    private:
        void task_impl();
        //actual detection logic
        void handleEncoderCallback(uint gpio, uint32_t events);

        QueueHandle_t to_CO2;
        QueueHandle_t to_Network;
        QueueHandle_t to_UI;
        QueueHandle_t encoder_queue;
        const char *name = "TEST";

        // For interrupt
        static QueueTest* instance;
        GPIO rotA;
        GPIO rotB;
        GPIO sw;
        TickType_t last_sw_time;


};



#endif //QUEUETEST_H
