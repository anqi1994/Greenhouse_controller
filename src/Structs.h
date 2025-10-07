#ifndef STRUCTS_H
#define STRUCTS_H
//all the structs that are needed in this project.

#define MIN_CO2_SET 500
#define MAX_CO2_SET 1500
#define CLOUD_CONNECTED_BIT (1<<0)
#define RECONNECT_WIFI_BIT (1<<1)

//monitored data gathered from the sensors
struct Monitored_data{
    uint16_t co2_val;
    double temperature;
    double humidity;
    uint16_t fan_speed;
};

//define the two types of messages to be sent to different queues
enum MessageType{
    MONITORED_DATA,
    CO2_SET_DATA,
    NETWORK_CONFIG,
};

struct NetworkConfig{
    char ssid[64];
    char password[64];
};

//combine message type and data
struct Message{
    MessageType type;

    Monitored_data data;
    uint co2_set;
    NetworkConfig network_config;
};

#endif //STRUCTS_H
