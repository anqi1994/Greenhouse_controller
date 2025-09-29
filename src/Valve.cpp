#include "Valve.h"

//valve set
Valve::Valve(uint8_t pin)
    //default mode: output, no pull-up, inverted
    : gpio(pin,false,false),opened(false){
    //close in the beginning
    close();
}

//turn on valve
void Valve::open(){
    opened = true;
    gpio.write(true);
}

//turn off valve
void Valve::close(){
    opened = false;
    gpio.write(false);
}

//check if the valve is opened
bool Valve::check_open() const{
    return opened;
}