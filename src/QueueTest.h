#ifndef QUEUETEST_H
#define QUEUETEST_H
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "GPIO/GPIO.h"
#include "ssd1306os.h"
#include "Structs.h"

enum encoderEv {
    ROT_L,
    ROT_R,
    SW,
};

enum screens {
    WELCOME,
    CO2_CHANGE,
    NET_SET,
    SSID,
    PASS,
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
        uint co2_set; // default value
        uint min_co2 = 500;
        uint max_co2 = 1990;
        uint co2_edit; // initially same as co2_set

        Message received;
        // For interrupt
        static QueueTest* instance;
        GPIO rotA;
        GPIO rotB;
        GPIO sw;
        TickType_t last_sw_time;

        //data for display
        // these pointers are to use display throughout functions in the class
        std::shared_ptr<PicoI2C> i2cbus1;
        std::unique_ptr<ssd1306os> display;

        uint oled_height = 64;
        uint oled_width = 128;
        uint line_height = 8;

        const char *welcome_menu[2] = { "SET CO2", "SET NETWORK" };
        const char *co2_menu[2] = { "SAVE", "BACK" };
        const char *network_menu[2] = { "CHANGE", "BACK" };

        uint current_menu_item = 0;
        uint menu_size = 2;
        screens current_screen = WELCOME;
        bool screen_needs_update = true;
        bool editing_co2 = false;

        // functions for display
        void display_screen();


};



#endif //QUEUETEST_H