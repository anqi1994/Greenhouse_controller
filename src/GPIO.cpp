#include "GPIO.h"

//initialization
GPIO::GPIO(uint8_t pin, bool input, bool pullup,bool invert)
    :pin_number(pin),is_input(input),is_pullup(pullup),is_inverted(invert)
{
    gpio_init(pin_number);

    if(is_input){
        gpio_set_dir(pin_number,GPIO_IN);
        if(is_pullup){
            gpio_pull_up(pin_number);
        }
        if(is_inverted){
            gpio_set_inover(pin_number,GPIO_OVERRIDE_INVERT);
        }
    }else{
        gpio_set_dir(pin_number,GPIO_OUT);
    }
}

//read from input
bool GPIO::read() const{
        return gpio_get(pin_number);
}

//write to output
void GPIO::write(bool value){
    if(!is_input){
        gpio_put(pin_number,value);
    }
}

