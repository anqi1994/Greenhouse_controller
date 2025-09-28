//
// Created by lily on 9/19/2025.
//
#include "global_definition.h"

// Definitions of Global Variables
SemaphoreHandle_t gpio_sem = NULL;
QueueHandle_t setpoint_q = NULL;
QueueHandle_t wifiCharPos_q = NULL;
EventGroupHandle_t wifiEventGroup = NULL;
EventGroupHandle_t co2EventGroup = NULL;
TimerHandle_t cloudSender_T = NULL;
TimerHandle_t cloudReceiver_T = NULL;

int co2 = 200;
int temp = 20;
int rh = 20;
int speed = 0;
uint16_t setpoint = 200;
uint16_t setpoint_temp = setpoint;
uint selection_screen_option = 0;
uint wifi_screen_option = 0;
bool isEditing = false;

char SSID_WIFI[64] = {0};
char PASS_WIFI[64] = {0};

rotBtnData_str rot_btn_data = { BACK_PRESS, SELECTION_SCR};
wifi_char_pos_str wifi_char_pos_X = {44, 32};
QueueHandle_t gpio_action_q = NULL;
QueueHandle_t gpio_data_q = NULL;