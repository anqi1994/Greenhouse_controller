#include "Control.h"


Control::Control(SemaphoreHandle_t timer, QueueHandle_t to_UI, QueueHandle_t to_Network, QueueHandle_t to_CO2,uint32_t stack_size, UBaseType_t priority) :
    timer_semphr(timer), to_UI(to_UI), to_Network(to_Network) ,to_CO2 (to_CO2){

    xTaskCreate(task_wrap, name, stack_size, this, priority, nullptr);
}

void Control::task_wrap(void *pvParameters) {
    auto *control = static_cast<Control*>(pvParameters);
    control->task_impl();
}

void Control::task_impl() {
    // protocol initialization
    auto uart = std::make_shared<PicoOsUart>(UART_NR, UART_TX_PIN, UART_RX_PIN, BAUD_RATE, STOP_BITS);
    auto rtu_client = std::make_shared<ModbusClient>(uart);
    auto i2cbus1 = std::make_shared<PicoI2C>(1, 100000);


    // sensor objects
    //CO2Sensor co2(rtu_client, 240, false);
    GMP252 co2(rtu_client, 240);
    HMP60 tem_hum_sensor(rtu_client,241);
    SDP610 pressure_sensor(i2cbus1);

    //actuators: valve and fan
    Valve valve(27);
    Produal fan(rtu_client, 1);
    //this is just for testing, co2_set needs to be retrieved from UI/network.
    uint co2_set = 1500;

    while(true) {
        //monitored data and message type are saved in structs.h
        Monitored_data data{};
        MessageType msg = MONITORED_DATA;
        Message message{};
        uint received_data;

        //main CO2 control logic which is triggered by the timer for getting monitored data.
        if (xSemaphoreTake(timer_semphr, portMAX_DELAY) == pdTRUE) {
            /*int co2_val = co2.readPpm();
            if (co2.isWorking()) {
                printf("CO2: %d ppm\n", co2_val);
            } else {
                printf("CO2 failed, last value: %d ppm\n", co2.lastPpm());
            }*/

            //getting monitored data from the sensors (GMP252- CO2, HMP60 -RH & TEM) -without Error checking
            data.co2_val = co2.read_value();
            printf("co2_val: %u\n", data.co2_val);
            //vTaskDelay(pdMS_TO_TICKS(100));
            data.temperature = tem_hum_sensor.read_tem();
            printf("temperature: %.1f\n", data.temperature);
            //vTaskDelay(pdMS_TO_TICKS(100));
            data.humidity = tem_hum_sensor.read_hum();
            printf("humidity: %.1f\n", data.humidity);
            data.pressure = pressure_sensor.read();
            printf("pressure: %.1f\n", data.pressure);
            //vTaskDelay(pdMS_TO_TICKS(100));
            message.type = msg;
            message.data = data;
            xQueueSendToBack(to_UI, &message, portMAX_DELAY);
            printf("QUEUE from co2 to UI\n");
            xQueueSendToBack(to_Network, &message, portMAX_DELAY);
            printf("QUEUE from co2 to network\n");

            //main co2 level control logic
            if (data.co2_val >= max_co2) {
                //set the fan to 50% when it reaches max_co2
                fan.setSpeed(50);
                vTaskDelay(pdMS_TO_TICKS(100));
                if(!check_fan(fan)){
                    //fan is not working
                    printf("fan failed\n");
                    fan_working = false;
                }
            } else{
                fan.setSpeed(0);
            }

            if(data.co2_val <= co2_set) {
                //need to add more in this section to test if valve works.
                valve.open();
                vTaskDelay(pdMS_TO_TICKS(1000));
                valve.close();
            } else {
                valve.close();
            }
        }

        //get data from UI and network. data type received: only uint CO2 set level.
        if(xQueueReceive(to_CO2, &received_data, 0) == pdTRUE){
            if(received_data < max_co2){
                co2_set = received_data;
                printf("co2: %u\n", received_data);
            }else
            {
                printf("co2 set is not in acceptable range.\n");
            }
        }

    }
}

//check if fan is running after two reads
bool Control::check_fan(Produal &fan){
    int first = fan.returnPulse();
    printf("fan.returnPulse(): %d\n", first);
    if(first == 0 && fan.getSpeed() > 0){
        //give time for the second read
        vTaskDelay(pdMS_TO_TICKS(100));
        if(fan.returnPulse() == 0){
            return false; //fan is not working
        }
    }
    return true; //fan is working
}
