#ifndef QUEUETESTTWO_H
#define QUEUETESTTWO_H
#include "../../FreeRTOS-KernelV10.6.2/include/FreeRTOS.h"
#include "../../FreeRTOS-KernelV10.6.2/include/queue.h"
#include "../../FreeRTOS-KernelV10.6.2/include/task.h"
#include "../ipstack/IPStack.h"
#include "../Structs.h"
#include <event_groups.h>


class Network {
public:
    Network(QueueHandle_t to_CO2, QueueHandle_t to_UI, QueueHandle_t to_Network, EventGroupHandle_t network_event_group, uint32_t stack_size = 2048, UBaseType_t priority = tskIDLE_PRIORITY + 2);
    static void task_wrap(void *pvParameters);
    char* extract_thingspeak_http_body();

private:
    void task_impl();
    void load_wifi_cred();
    bool connect_to_http(IPStack &ip_stack);
    int disconnect_to_http(IPStack &ip_stack);
    bool upload_sensor_data(IPStack &ip_stack, Monitored_data &data);
    bool upload_co2_set_level(IPStack &ip_stack, uint co2_set);
    uint read_co2_set_level(IPStack &ip_stack);
    bool connect_to_cloud(IPStack &ip_stack, const char* wifi_ssid, const char* wifi_password);
    QueueHandle_t to_CO2;
    QueueHandle_t to_UI;
    QueueHandle_t to_Network;
    const char *name = "TESTTWO";
    const char *host = "api.thingspeak.com";
    const int port = 80; //http service
    const char *write_api = "7RC0GM5VZK7VRPN7";
    const char *read_api = "9L9GPCBA6QG1ZC14";
    static const int BUFSIZE = 2048;
    char buffer[BUFSIZE];
    const char *wifissid;
    const char *wifipass;

    bool wifi_connected = false;
    bool http_connected = false;
    EventGroupHandle_t network_event_group;

};

#endif //QUEUETEST_H
