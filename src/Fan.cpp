#include "Fan.h"

// addresses are wire addresses (numbering starts from zero)
Fan::Fan(std::shared_ptr<ModbusClient> client, int server_address)
    :produal_speed(client,server_address,0,true), //A01, holding register (R&W), register address: 40001
    produal_pulse(client,server_address,4,false) //AL1 digital counter (Input register), register address: 30005
    {}

//set the Speed of the fan
void Fan::setSpeed(int speed){
    if (speed < 0) speed = 0;
    if (speed > 100) speed = 100;
    produal_speed.write(speed);
    current_speed = speed;
}

//check if fan is running after two reads
bool Fan::isWorking(){
    int first = produal_pulse.read();
    if(first == 0 && current_speed > 0){
        vTaskDelay(pdMS_TO_TICKS(100));
        if(produal_pulse.read() == 0){
            is_working = false;
            return is_working;
        };
    }
    is_working = true;
    return is_working;
}

int Fan::getSpeed() const{
    return current_speed;
}

bool Fan::checkWorkingStatus() const{
    return is_working;
}