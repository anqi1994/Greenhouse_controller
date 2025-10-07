#ifndef UI_H
#define UI_H
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "GPIO/GPIO.h"
#include "ssd1306os.h"
#include "Structs.h"
#include <event_groups.h>
#include <cstring>

enum encoderEv {
    ROT_L,
    ROT_R,
    SW,
    DONE,
};

enum screens {
    WELCOME,
    CO2_CHANGE,
    NET_SET,
    SSID,
    PASS,
};

class UI {
    public:
        UI(QueueHandle_t to_CO2, QueueHandle_t to_Network, QueueHandle_t to_UI, EventGroupHandle_t network_event_group,
                uint32_t stack_size = 2048, UBaseType_t priority = tskIDLE_PRIORITY + 1);

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
        uint co2_set;
        uint min_co2 = MIN_CO2_SET;
        uint max_co2 = MAX_CO2_SET;
        uint co2_edit;

        const char *conn_status;

        Message received;
        // For interrupt
        static UI* instance;
        GPIO rotA;
        GPIO rotB;
        GPIO sw;
        GPIO done_button;
        TickType_t last_sw_time = 0;
        // these are used so that entering ssid and pass exits when pressed longer
        TickType_t last_done_time = 0;


        //data for display
        // these pointers are to use display throughout functions in the class
        std::shared_ptr<PicoI2C> i2cbus1;
        std::unique_ptr<ssd1306os> display;

        uint oled_height = 64;
        uint oled_width = 128;
        uint line_height = 8;

        const char *welcome_menu[2] = { "SET CO2", "SET NETWORK" };
        const char *co2_menu[2] = { "SAVE", "BACK" };
        const char *network_menu[2] = { "NEW CONN.", "BACK" };
        const char *text_entry_menu[2] = { "DELETE LAST", "OK" };

        uint current_menu_item = 0;
        //uint menu_size = 2;
        screens current_screen = WELCOME;
        bool screen_needs_update = true;
        bool editing_co2 = false;

        // variables to use when setting network
        static constexpr uint MAX_SSID_LENGTH = 32;
        static constexpr uint MAX_PASSWORD_LENGTH = 32;

        const char* ascii_chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        uint ascii_length = 62;

        std::string ssid_input;
        std::string pass_input;
        uint current_char_id = 0;
        bool selecting_char = true;
        EventGroupHandle_t network_event_group;


        // functions for display
        // different screen display handle
        void welcome_screen(encoderEv ev);
        void co2_screen(encoderEv ev);
        void network_screen(encoderEv ev);
        void text_entry_screen(encoderEv ev, std::string& input_str, uint max_len, screens next_screen);

        void display_screen();
        void navigate_menu(encoderEv ev);
        void reset_text_entry();
        void change_screen(screens new_screen);
};



#endif //UI_H