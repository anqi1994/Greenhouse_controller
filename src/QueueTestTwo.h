#ifndef QUEUETESTTWO_H
#define QUEUETESTTWO_H
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "ipstack/IPStack.h"
#include "Structs.h"

#define SSID "Julijaiph"
#define PASSWORD "12341234"

class QueueTestTwo {
public:
    QueueTestTwo(QueueHandle_t to_CO2, QueueHandle_t to_UI, QueueHandle_t to_Network, uint32_t stack_size = 2048, UBaseType_t priority = tskIDLE_PRIORITY + 2);
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
    const char *wifissid = "Julijaiph";
    const char *wifipass = "12341234";
    bool wifi_connected = false;
    bool http_connected = false;
};

#endif //QUEUETEST_H
