#ifndef STRUCTS_H
#define STRUCTS_H
//all the structs that are needed in this project.

//monitored data gathered from the sensors
struct Monitored_data{
    uint16_t co2_val;
    double temperature;
    double humidity;
    uint16_t pressure;
};

//define the two types of messages to be sent to different queues
enum MessageType{
    MONITORED_DATA,
    CO2_SET_DATA
};

//combine message type and data
struct Message{
    MessageType type;

    Monitored_data data;
    uint co2_set;
};

#endif //STRUCTS_H
