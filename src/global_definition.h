//
// Created by lily on 9/19/2025.
//

#ifndef FREERTOS_VENTILATION_PROJECT_GLOBAL_DEFINITION_H
#define FREERTOS_VENTILATION_PROJECT_GLOBAL_DEFINITION_H

#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"
#include "event_groups.h"
#include "timers.h"
#define QUEUE_SIZE 10

// Screen Types
typedef enum {
    WELCOME_SCR,
    SELECTION_SCR,
    INFO_SCR,
    WIFI_CONF_SCR,
    SET_CO2_SCR
} screen_t;

// Global Variables Declarations (with 'extern')
extern SemaphoreHandle_t gpio_sem;
extern QueueHandle_t setpoint_q;
extern QueueHandle_t screenOption_q;
extern EventGroupHandle_t wifiEventGroup;
extern EventGroupHandle_t co2EventGroup;
extern TimerHandle_t cloudSender_T;
extern TimerHandle_t cloudReceiver_T;

extern int co2;
extern int temp;
extern int rh;
extern int speed;
extern uint16_t setpoint;
extern uint16_t setpoint_temp;

extern char SSID_WIFI[64];
extern char PASS_WIFI[64];

#define SEND_INTERVAL 60000  // 1m
#define RECEIVE_INTERVAL 10000 // 10s

#define WIFI_CHANGE_BIT_TLS (1 << 0)  // Bit 0 for tls_task
#define WIFI_CHANGE_BIT_EEPROM (1 << 1)  // Bit 1 for eeprom_task
#define CO2_CHANGE_BIT_EEPROM (1 << 0) //notify if new co2 setpoint for eeprom
#define CO2_CHANGE_BIT_LCD (1 << 1) //notify if new co2 setpoint for lcd

#define TLS_CLIENT_SERVER "api.thingspeak.com"
#define TLS_CLIENT_TIMEOUT_SECS 15
#define API_KEY "..."
#define TALKBACK_KEY "..."

// Screen Options
extern uint selection_screen_option;
extern uint wifi_screen_option;
extern QueueHandle_t wifiCharPos_q;
extern bool isEditing;

// Pin Definitions
#define rotA 10
#define rotB 11
#define led1 22
#define valve 27
#define ok_btn 9
#define back_btn 7

// Encoder Actions
typedef enum {
    ROT_CW,
    ROT_CCW,
    OK_PRESS,
    BACK_PRESS
} gpio_t;

// Encoder Data Structure
typedef struct {
    gpio_t action_type;
    screen_t screen_type;
} rotBtnData_str;

typedef struct {
    int pos_x;
    int ch;
} wifi_char_pos_str;

extern wifi_char_pos_str wifi_char_pos_X;
extern rotBtnData_str rot_btn_data;
extern QueueHandle_t gpio_action_q;
extern QueueHandle_t gpio_data_q;

// UART Settings
#if 0
#define UART_NR 0
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#else
#define UART_NR 1
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#endif

#define BAUD_RATE 9600
#define STOP_BITS 2  // For real system (Pico simulator also requires 2 stop bits)


// LED Parameters Structure
struct led_params {
    uint pin;
    uint delay;
};

#define SSID_POS_X 44
#define PASS_POS_X 44

#define MIN_SETPOINT 200
#define MAX_SETPOINT 1500
#define CO2_LIMIT 2000
#define OFFSET 100


#endif //FREERTOS_VENTILATION_PROJECT_GLOBAL_DEFINITION_H


