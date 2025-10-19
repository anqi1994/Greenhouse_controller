#include "Control.h"

//todo: maybe use a state machine for cleaner task_impl?
//todo: could be a function for sending initial eeprom

Control::Control(SemaphoreHandle_t timer,
    QueueHandle_t to_UI, QueueHandle_t to_Network, QueueHandle_t to_CO2,
    EventGroupHandle_t network_event_group,
    uint32_t stack_size,
    UBaseType_t priority) :
    timer_semphr(timer), to_UI(to_UI), to_Network(to_Network) ,to_CO2 (to_CO2),network_event_group(network_event_group){

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
    //auto i2cbus1 = std::make_shared<PicoI2C>(1, 100000); for pressure, but not used
    auto i2cbus0 = std::make_shared<PicoI2C>(0, 100000);

    // sensor objects
    GMP252 co2(rtu_client, 240);
    HMP60 tem_hum_sensor(rtu_client,241);
    //SDP610 pressure_sensor(i2cbus0); // done but not used here

    //actuators: valve and fan
    Valve valve(27);
    Produal fan(rtu_client, 1);

    // EEPROM extern memory
    eeprom = std::make_shared<EEPROM>(i2cbus0);

    // check last eeprom data
    rebooted = false;
    check_last_eeprom_data(&last_co2_set, &last_fan_speed, &rebooted, wifi_ssid, wifi_pass);

    if (rebooted) {
        printf("UNEXPECTED REBOOT\n");

        fan.setSpeed(0);
        //also send last wifi credentials if rebooted
        Message wifi_message;
        wifi_message.type = NETWORK_CONFIG;
        strcpy(wifi_message.network_config.ssid, wifi_ssid);
        strcpy(wifi_message.network_config.password, wifi_pass);
        printf("FROM EEPROM ssid: %s\n", wifi_message.network_config.ssid);
        printf("FROM EEPROM password: %s\n", wifi_message.network_config.password);
        xQueueSendToBack(to_Network, &wifi_message, portMAX_DELAY);
        xEventGroupSetBits(network_event_group, RECONNECT_WIFI_BIT);
        printf("Wifi details sent to network\n");
    }
    // las co2 set value sent to network and UI
    Message co2_eeprom;
    co2_eeprom.type = CO2_SET_DATA;
    co2_eeprom.co2_set = last_co2_set;
    xQueueSendToBack(to_UI, &co2_eeprom, portMAX_DELAY);
    xQueueSendToBack(to_Network, &co2_eeprom, portMAX_DELAY);
    printf("EEPROM CO2: %u\n", last_co2_set);
    uint co2_set = last_co2_set;

    Message from_eeprom;
    from_eeprom.type = MONITORED_DATA;
    // initial data (set co2 and last fan speed) is sent from eeprom as last read values. Other values are displayed
    //as 0s until measured again after reboot. Was done to be able to retrieve required data after reboot
    from_eeprom.data.co2_val = 0;
    from_eeprom.data.humidity = 0;
    from_eeprom.data.temperature = 0;
    from_eeprom.data.fan_speed = 0;
    xQueueSendToBack(to_UI, &from_eeprom, portMAX_DELAY);


    TickType_t last_valve_time = xTaskGetTickCount();
    bool valve_open = false;

    while(true) {
        //monitored data and message type are saved in structs.h
        Monitored_data data{};
        MessageType msg = MONITORED_DATA;
        Message message{};
        Message received;

        //main CO2 control logic which is triggered by the timer for getting monitored data.
        if (xSemaphoreTake(timer_semphr, pdMS_TO_TICKS(10)) == pdTRUE) {
            eeprom->printAllLogs();
            //getting monitored data from the sensors (GMP252- CO2, HMP60 -RH & TEM) -without Error checking
            data.co2_val = co2.read_value();
            printf("co2_val: %u\n", data.co2_val);
            if(data.co2_val == 0){
                eeprom->writeLog("co2 measure failed this round");
            }else{
                eeprom->writeLog("co2 measured");
            }
            //vTaskDelay(pdMS_TO_TICKS(10));
            data.temperature = tem_hum_sensor.read_tem();
            printf("temperature: %.1f\n", data.temperature);
            eeprom->writeLog("temp measured");
            vTaskDelay(pdMS_TO_TICKS(10));
            data.humidity = tem_hum_sensor.read_hum();
            printf("humidity: %.1f\n", data.humidity);
            if(data.humidity == 0){
                //humidity and temperature use the same sensor
                eeprom->writeLog("T&RH measure failed this round");
            }else{
                eeprom->writeLog("humidity measured");
            }

            data.fan_speed = fan.getSpeed();
            printf("fan_speed: %u\n", data.fan_speed);

            eeprom->writeLog("pressure measured");
            message.type = msg;
            message.data = data;


            //main co2 level control logic
            handle_fan_control(fan, data.co2_val, max_co2, co2_set);

            snprintf(status_buffer, sizeof(status_buffer), "%u", fan.getSpeed());
            eeprom->writeStatus(FAN_SPEED_ADDR, status_buffer);

            message.data.fan_speed = fan.getSpeed();
            // send data to queues from co2 control task
            xQueueSendToBack(to_UI, &message, portMAX_DELAY);

            EventBits_t bits = xEventGroupGetBits(network_event_group);

            //only send information to network once it is connected to the cloud
            if (bits & CLOUD_CONNECTED_BIT){
                xQueueSendToBack(to_Network, &message, portMAX_DELAY);
            }

            // a minute at least between openings
            if(data.co2_val <= co2_set) {
                if (!valve_open) {
                    valve.open();
                    printf("valve open\n");
                    eeprom->writeLog("valve open");

                    //following is for real system,open the valve only for 0.5s!
                    vTaskDelay(pdMS_TO_TICKS(500));

                    valve.close();
                    eeprom->writeLog("valve closed");
                    valve_open = true;
                    last_valve_time = xTaskGetTickCount();
                } else {
                    TickType_t current_time = xTaskGetTickCount();
                    // at least a minute interval between valve opening again to let co2 spread
                    if (current_time - last_valve_time > pdMS_TO_TICKS(30000)) {
                        valve_open = false;
                    }
                }
            }
        }

        //get data from UI and network
        if(xQueueReceive(to_CO2, &received, 0) == pdTRUE){
            if (received.type == CO2_SET_DATA) {
                if(received.co2_set < max_co2){
                    co2_set = received.co2_set;
                    snprintf(status_buffer, sizeof(status_buffer), "%u", co2_set);
                    if (eeprom->writeStatus(CO2_SET_ADDR, status_buffer)) {
                        printf("co2 set val written to EEPROM\n");
                        eeprom->writeLog("co2 val changed");
                    }
                    printf("CONTROL co2: %u\n", received.co2_set);
                }
            } else if (received.type == NETWORK_CONFIG) {
                strcpy(wifi_ssid, received.network_config.ssid);
                eeprom->writeStatus(WIFI_SSID_ADDR, wifi_ssid, STR_BUFFER_SIZE);

                strcpy(wifi_pass,received.network_config.password);
                eeprom->writeStatus(WIFI_PASS_ADDR, wifi_pass, STR_BUFFER_SIZE);
            }
        }
    }
}

void Control::handle_fan_control(Produal &fan, uint16_t co2_level, uint16_t max_co2, uint16_t set_co2) {
    if (co2_level >= max_co2) {
        fan.setSpeed(max_fan_speed);
        vTaskDelay(pdMS_TO_TICKS(10));

        if(!check_fan(fan)) {
            eeprom->writeLog("Fan failed");
        }
    } else if (co2_level <= set_co2) {
        fan.setSpeed(0);
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

void Control::check_last_eeprom_data(uint16_t *last_co2_set, uint16_t *last_fan_speed, bool *rebooted,
    char *wifi_ssid, char *wifi_pass) {
    // eeprom checking last saved data
    eeprom->readStatus(REBOOT_ADDR, status_buffer, STATUS_BUFF_SIZE);
    //check if the system turned of while running
    if (strcmp(status_buffer, RUN_FLAG) == 0) {
        *rebooted = true;
        eeprom->writeLog("Unexpected system shutdown");
    } else if (strcmp(status_buffer, REBOOT_FLAG) == 0) {
        *rebooted = false;
        eeprom->writeLog("Normal system start");
    } else {
        //if first time or data corrupted
        *rebooted = false;
        eeprom->writeLog("First system start");
    }
    // mark system as running
    eeprom->writeStatus(REBOOT_ADDR, RUN_FLAG);

    eeprom->readStatus(CO2_SET_ADDR, status_buffer, STATUS_BUFF_SIZE);
    *last_co2_set = atoi(status_buffer);
    if (*last_co2_set < 500 || *last_co2_set > 1500) {
        clearEEPROM();// shouldn't be anything outside these values cause they are restricted in UI and Netw
        *last_co2_set = 700;
    }

    // read last saved wifi and pass
    eeprom->readStatus(WIFI_SSID_ADDR, string_buffer, STR_BUFFER_SIZE, STR_BUFFER_SIZE);
    strcpy(wifi_ssid, string_buffer);

    eeprom->readStatus(WIFI_PASS_ADDR, string_buffer, STR_BUFFER_SIZE, STR_BUFFER_SIZE);
    strcpy(wifi_pass, string_buffer);
}

void Control::clearEEPROM() {
    printf("Clearing entire EEPROM...\n");

    const size_t CHUNK_SIZE = 32;
    uint8_t zero_buffer[CHUNK_SIZE] = {};

    // Adjust max address based on your EEPROM size (e.g., 0x7FFF for 32KB)
    for (uint16_t addr = 0; addr <= 0x7FFF; addr += CHUNK_SIZE) {
        eeprom->eepromWrite(addr, zero_buffer, CHUNK_SIZE);

        if (addr % 256 == 0) {
            printf("Cleared: 0x%04X\n", addr);
        }
    }
    printf("EEPROM cleared!\n");
}

