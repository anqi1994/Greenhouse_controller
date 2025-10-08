#ifndef CONTROL_H
#define CONTROL_H

#define UART_NR 1
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#define BAUD_RATE 9600
#define STOP_BITS 2

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "Fan/Produal.h"
#include "CO2_sensor/GMP252.h"
#include "T_RH_sensor/HMP60.h"
#include "Pressure_sensor/SDP610.h"
#include "Valve/Valve.h"
#include "Structs.h"
#include "EEPROM/EEPROM.h"
#include <event_groups.h>



class Control {
public:
    Control(SemaphoreHandle_t timer, QueueHandle_t to_UI, QueueHandle_t to_Network, QueueHandle_t to_CO2,EventGroupHandle_t network_event_group,uint32_t stack_size = 1024, UBaseType_t priority = tskIDLE_PRIORITY + 2);
    static void task_wrap(void *pvParameters);


private:
    // Private functions
    void task_impl();
    bool check_fan(Produal &fan);
    void handle_fan_control(Produal &fan, uint16_t co2_level, uint16_t max_co2);
    //bool check_sensor_val(Monitored_data data);
    void check_last_eeprom_data(uint16_t *last_co2_set, uint16_t *last_fan_speed, bool *rebooted,
        char *wifi_ssid, char *wifi_pass);
    void clearEEPROM();

    SemaphoreHandle_t timer_semphr;
    TaskHandle_t control_task;
    const char *name = "CONTROL";
    uint16_t max_co2 = 2000;
    bool fan_working = true;
    uint16_t max_fan_speed = 100;
    QueueHandle_t to_UI;
    QueueHandle_t to_Network;
    QueueHandle_t to_CO2;
    EventGroupHandle_t network_event_group;

    // VALUES FROM EEPROM
    std::shared_ptr<EEPROM> eeprom;
    char status_buffer[STATUS_BUFF_SIZE];
    char string_buffer[STR_BUFFER_SIZE];
    uint16_t last_co2_set;
    uint16_t last_fan_speed;
    bool rebooted;
    char wifi_ssid[STR_BUFFER_SIZE];
    char wifi_pass[STR_BUFFER_SIZE];


};

#endif //CONTROL_H
