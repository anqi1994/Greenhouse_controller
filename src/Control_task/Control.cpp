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
    auto i2cbus0 = std::make_shared<PicoI2C>(0, 100000);


    // sensor objects
    //CO2Sensor co2(rtu_client, 240, false);
    GMP252 co2(rtu_client, 240);
    HMP60 tem_hum_sensor(rtu_client,241);
    SDP610 pressure_sensor(i2cbus1);

    //actuators: valve and fan
    Valve valve(27);
    Produal fan(rtu_client, 1);

    // EEPROM memory chip
    EEPROM eeprom(i2cbus0);
    //this is just for testing, co2_set needs to be retrieved from UI/network.
    uint co2_set = 1500;
    TickType_t last_valve_time = xTaskGetTickCount();
    bool valve_open = false;

    while(true) {
        //monitored data and message type are saved in structs.h
        Monitored_data data{};
        MessageType msg = MONITORED_DATA;
        Message message{};
        uint received_data;

        //main CO2 control logic which is triggered by the timer for getting monitored data.
        if (xSemaphoreTake(timer_semphr, portMAX_DELAY) == pdTRUE) {
            //getting monitored data from the sensors (GMP252- CO2, HMP60 -RH & TEM) -without Error checking
            data.co2_val = co2.read_value();
            printf("co2_val: %u\n", data.co2_val);
            if(data.co2_val == 0){
                //eeprom.writeLog("co2 measure failed this round");
            }else{
                //eeprom.writeLog("co2 measured");
            }
            //vTaskDelay(pdMS_TO_TICKS(10));
            data.temperature = tem_hum_sensor.read_tem();
            printf("temperature: %.1f\n", data.temperature);
            //eeprom.writeLog("temp measured");
            //vTaskDelay(pdMS_TO_TICKS(10));
            data.humidity = tem_hum_sensor.read_hum();
            printf("humidity: %.1f\n", data.humidity);
            if(data.humidity == 0){
                //humidity and temperature use the same sensor
                //eeprom.writeLog("T&RH measure failed this round");
            }else{
                //eeprom.writeLog("humidity measured");
            }

            data.fan_speed = fan.getSpeed();
            printf("fan_speed: %u\n", data.fan_speed);

            //printf("pressure: %.1f\n", pressure_sensor.read());
            //eeprom.writeLog("pressure measured");
            //vTaskDelay(pdMS_TO_TICKS(10));
            message.type = msg;
            message.data = data;
            xQueueSendToBack(to_UI, &message, portMAX_DELAY);
            //printf("QUEUE from co2 to UI\n");
            xQueueSendToBack(to_Network, &message, portMAX_DELAY);
            //printf("QUEUE from co2 to network\n");

            //main co2 level control logic
            if (data.co2_val >= max_co2) {
                //set the fan to 100% when it reaches max_co2
                fan.setSpeed(100);
                vTaskDelay(pdMS_TO_TICKS(10));
                if(!check_fan(fan)){
                    //fan is not working
                    printf("fan failed\n");
                    fan_working = false;
                }
            } else{
                fan.setSpeed(0);
            }
            // a minute at least between openings
            if(data.co2_val <= co2_set) {
                if (!valve_open) {
                    valve.open();
                    printf("valve open\n");

                    //following is for real system,open the valve only for 0.5s!
                    vTaskDelay(pdMS_TO_TICKS(500));

                    //following is for test system!!!!! valve opens for 5s to make sure CO2 level goes up.
                    //vTaskDelay(pdMS_TO_TICKS(5000));
                    valve.close();
                    valve_open = true;
                    last_valve_time = xTaskGetTickCount();
                } else {
                    TickType_t current_time = xTaskGetTickCount();
                    // at least a minute interval between valve opening again to let co2 spread
                    if (current_time - last_valve_time > pdMS_TO_TICKS(60000)) {
                        valve_open = false;
                        //last_valve_time = current_time;
                    }
                }
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

        //eeprom.printAllLogs();
    }
}

//check if fan is running after two reads
bool Control::check_fan(Produal &fan){
    int first = fan.returnPulse();
    printf("fan.returnPulse(): %d\n", first);
    if(first == 0 && fan.getSpeed() > 0){
        //give time for the second read
        vTaskDelay(pdMS_TO_TICKS(10));
        if(fan.returnPulse() == 0){
            return false; //fan is not working
        }
    }
    return true; //fan is working
}