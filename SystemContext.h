#ifndef FREERTOS_VENTILATION_PROJECT_SYSTEMCONTEXT_H
#define FREERTOS_VENTILATION_PROJECT_SYSTEMCONTEXT_H

#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"
#include "event_groups.h"
#include "timers.h"
#include <cstdint>
#include <string>

// ----------- ENUMS -----------

// Screen Types
enum class ScreenType {
    WELCOME_SCR,
    SELECTION_SCR,
    INFO_SCR,
    WIFI_CONF_SCR,
    SET_CO2_SCR
};

// Encoder Actions
enum class GpioAction {
    ROT_CW,
    ROT_CCW,
    OK_PRESS,
    BACK_PRESS
};

// ----------- STRUCTS -----------

// Encoder Data Structure
struct RotBtnData {
    GpioAction action_type;
    ScreenType screen_type;
};

// Wifi Character Position
struct WifiCharPos {
    int pos_x;
    int ch;
};

// LED Parameters Structure
struct LedParams {
    uint pin;
    uint delay;
};

// ----------- CONTEXT CLASS -----------

class SystemContext {
public:
    // Singleton accessor
    static SystemContext& getInstance() {
        static SystemContext instance;
        return instance;
    }

    // Delete copy
    SystemContext(const SystemContext&) = delete;
    void operator=(const SystemContext&) = delete;

    // Queues, Semaphores, Event Groups
    SemaphoreHandle_t gpio_sem;
    QueueHandle_t setpoint_q;
    QueueHandle_t gpio_action_q;
    QueueHandle_t gpio_data_q;
    QueueHandle_t wifiCharPos_q;

    EventGroupHandle_t wifiEventGroup;
    EventGroupHandle_t co2EventGroup;

    TimerHandle_t cloudSender_T;
    TimerHandle_t cloudReceiver_T;

    // Runtime state variables
    int co2;
    int temp;
    int rh;
    int speed;

    uint16_t setpoint;
    uint16_t setpoint_temp;

    char SSID_WIFI[64];
    char PASS_WIFI[64];

    unsigned int selection_screen_option;
    unsigned int wifi_screen_option;

    WifiCharPos wifi_char_pos_X;
    RotBtnData rot_btn_data;

    bool isEditing;

    // Constants
    static constexpr int QUEUE_SIZE = 10;
    static constexpr int SEND_INTERVAL = 60000;
    static constexpr int RECEIVE_INTERVAL = 10000;

    static constexpr int SSID_POS_X = 44;
    static constexpr int PASS_POS_X = 44;
    static constexpr int MIN_SETPOINT = 200;
    static constexpr int MAX_SETPOINT = 1500;
    static constexpr int CO2_LIMIT = 2000;
    static constexpr int OFFSET = 100;

    // WiFi Event Bits
    static constexpr int WIFI_CHANGE_BIT_TLS = (1 << 0);
    static constexpr int WIFI_CHANGE_BIT_EEPROM = (1 << 1);

    // CO2 Event Bits
    static constexpr int CO2_CHANGE_BIT_EEPROM = (1 << 0);
    static constexpr int CO2_CHANGE_BIT_LCD = (1 << 1);

    // UART Settings
#if 0
    static constexpr int UART_NR = 0;
    static constexpr int UART_TX_PIN = 0;
    static constexpr int UART_RX_PIN = 1;
#else
    static constexpr int UART_NR = 1;
    static constexpr int UART_TX_PIN = 4;
    static constexpr int UART_RX_PIN = 5;
#endif
    static constexpr int BAUD_RATE = 9600;
    static constexpr int STOP_BITS = 2;

    // Pin Definitions
    static constexpr int rotA = 10;
    static constexpr int rotB = 11;
    static constexpr int led1 = 22;
    static constexpr int valve = 27;
    static constexpr int ok_btn = 9;
    static constexpr int back_btn = 7;

private:
    // Private constructor
    SystemContext() :
        gpio_sem(nullptr),
        setpoint_q(nullptr),
        gpio_action_q(nullptr),
        gpio_data_q(nullptr),
        wifiCharPos_q(nullptr),
        wifiEventGroup(nullptr),
        co2EventGroup(nullptr),
        cloudSender_T(nullptr),
        cloudReceiver_T(nullptr),
        co2(0), temp(0), rh(0), speed(0),
        setpoint(400), setpoint_temp(400),
        selection_screen_option(0), wifi_screen_option(0),
        isEditing(false)
    {
        SSID_WIFI[0] = '\0';
        PASS_WIFI[0] = '\0';
    }
};

#endif //FREERTOS_VENTILATION_PROJECT_SYSTEMCONTEXT_H
