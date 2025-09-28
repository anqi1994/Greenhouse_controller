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
    produal_speed.write(speed * 10);
    current_speed_in_percent = speed;
}

//return the speed of the fan saved in the system
int Fan::getSpeed() const{
    return current_speed_in_percent;
}

//get the pulse from the fan
uint16_t Fan::returnPulse(){
    return produal_pulse.read();
}

//check if fan is running after two reads
//bool fan_working = true;

/*bool checkFan(){
    uint16_t first = fan.returnPulse();
    if(first == 0 && fan.returnPulse() > 0){
        vTaskDelay(pdMS_TO_TICKS(100));
        if(fan.returnPulse == 0){
            return false;
        };
    return true;
}*/