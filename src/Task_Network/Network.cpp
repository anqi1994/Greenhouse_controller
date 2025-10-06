#include "Network.h"

#include <cstdio>
#include <cstring>


Network::Network(QueueHandle_t to_CO2,  QueueHandle_t to_UI, QueueHandle_t to_Network,EventGroupHandle_t network_event_group,uint32_t stack_size, UBaseType_t priority):
    to_CO2(to_CO2),to_UI (to_UI),to_Network(to_Network),network_event_group(network_event_group){

    //load_wifi_cred();
    xTaskCreate(task_wrap, name, stack_size, this, priority, nullptr);
}

void Network::task_wrap(void *pvParameters) {
    auto *test = static_cast<Network*>(pvParameters);
    test->task_impl();
}

void Network::task_impl() {
    //clear the event bits: cloud connected bit from the previous rounds
    xEventGroupClearBits(network_event_group, CLOUD_CONNECTED_BIT);

    Message received{};
    Message send{};

    //todo: this is a safe co2 set value but we need to get it from eeprom
    uint co2_set = 1000;
    Monitored_data monitored_data{};
    IPStack ip_stack;
    bool initial_data_ready = false;

    while (true) {
        //get data from CO2_control and UI. data type received: 1. monitored data. 2. uint CO2 set level. 3. network config
        if (xQueueReceive(to_Network, &received, pdMS_TO_TICKS(10))) {
            //the received data is from CO2_control_task
            if(received.type == MONITORED_DATA){
                printf("QUEUE to Network from CO2_control_task: co2: %d\n", received.data.co2_val);
                printf("thingspeak: temp: %.1f\n", received.data.temperature);
                //save the data from sensor readings
                monitored_data.co2_val = received.data.co2_val;
                monitored_data.temperature = received.data.temperature;
                monitored_data.humidity = received.data.humidity;
                monitored_data.fan_speed = received.data.fan_speed;
                send.type = MONITORED_DATA;
                send.data = monitored_data;
                initial_data_ready = true;
            }


            //the received data is from UI task
            else if(received.type == CO2_SET_DATA){
                //save the co2 set level from the UI task
                co2_set = received.co2_set;
                //initial_data_ready = true;
                //upload_co2_set_level(ip_stack, co2_set);
                //upload_sensor_data(ip_stack,monitored_data);
                //printf("QUEUE to network from UI: co2_set: %d\n", co2_set);
            }

            //the received data is from UI task or Control task (after reboot when eeprom has the information saved)
            else if(received.type == NETWORK_CONFIG){
                wifissid= received.network_config.ssid;
                wifipass= received.network_config.password;
                printf("received ssid: %s, password: %s",wifissid,wifipass);
                if(connect_to_cloud(ip_stack,wifissid,wifipass)){
                    //if cloud is connected, then set the network event group bit as 1
                    xEventGroupSetBits(network_event_group,CLOUD_CONNECTED_BIT);
                    //clear the talkback queue from previously saved data.
                    while(read_co2_set_level(ip_stack)) vTaskDelay(pdMS_TO_TICKS(10));
                };
            }
        }

        EventBits_t bits = xEventGroupGetBits(network_event_group);
        if (bits & CLOUD_CONNECTED_BIT && initial_data_ready) {
            if(ip_stack.WiFi_connected()){
                //retrieve co2 set level from talkback queue if there is any
                Message send_msg{};
                send_msg.type = CO2_SET_DATA;
                uint tem = read_co2_set_level(ip_stack);
                if( 500 < tem && tem  < 2000){
                    send_msg.co2_set = tem;
                    printf("co2_set value from network class: %u",tem);
                    //sending co2 set level from network to both UI and CO2 queue
                    //todo: need to update eeprom in control class
                    xQueueSendToBack(to_UI, &send_msg, pdMS_TO_TICKS(10));
                    xQueueSendToBack(to_CO2, &send_msg, pdMS_TO_TICKS(10));
                }

                //upload data to all fields in thingspeak once in 20s
                bool ok = upload_data_to_cloud(ip_stack,monitored_data,co2_set);
                if(ok){
                    printf("Upload success.\n");
                }else{
                    printf("Failed to upload.\n");
                }
                //delay for 15s as data can be uploaded to thingspeak once in 15s
                vTaskDelay(pdMS_TO_TICKS(15000));
            }else{
                xEventGroupClearBits(network_event_group, CLOUD_CONNECTED_BIT);
                printf("Connection lost detected, event bit reset.\n");
            }
        }
    }

    }


//connect to thingspeak service via http
bool Network::connect_to_http(IPStack &ip_stack){
    printf("%s,%d",host,port);
    int rc = ip_stack.connect(host, port);
    if(rc == 0){
        return true;
    }
    return false;
}

bool Network::connect_to_cloud(IPStack &ip_stack, const char* wifi_ssid, const char* wifi_password){
    wifi_connected = false;
    http_connected = false;

    //checks if wifi connection is successful with 5 times trial
    if(ip_stack.connect_WiFi(wifi_ssid, wifi_password,5)){
        wifi_connected= true;
    }

    //check if http connection is successful and quickly disconnect it.
    if(wifi_connected){
        if(connect_to_http(ip_stack)){
            http_connected = true;
            //disconnect_to_http(ip_stack);
        }
    }

    return (wifi_connected && http_connected);
}

//upload monitored data & co2_set to the sensor
bool Network::upload_data_to_cloud(IPStack &ip_stack, Monitored_data &data,uint co2_set){
    char req[300];

    // Update fields using a minimal GET request - tested to work
    //uploading monitored data to the cloud
    snprintf(req,sizeof(req),
            "GET /update?api_key=%s&field1=%u&field2=%.2f&field3=%.2f&field4=%u&field5=%u HTTP/1.1\r\n"
            "Host: %s\r\n"
            "\r\n",
            write_api,
            data.co2_val,
            data.temperature,
            data.humidity,
            data.fan_speed,
            co2_set,
            host);

    ip_stack.write((unsigned char *)(req),strlen(req));
    vTaskDelay(pdMS_TO_TICKS(2000));
    auto rv = ip_stack.read((unsigned char*)buffer, BUFSIZE, 100);
    if(rv <= 0){
        printf("No response from server\n");
        return false;
    }
    buffer[rv] = 0;
    char* body = extract_thingspeak_http_body();
    printf("%s\n",body);
    if(body){
        int id = atoi(body);
        if(id > 0){
            printf("upload monitored data to network. \n");
            return true;
        }
    }
    printf("upload monitored data to network failed. \n");
    return false;
}

//upload co2 set level
/*bool Network::upload_co2_set_level(IPStack &ip_stack, uint co2_set){
    char req[256];

    // Update fields using a minimal GET request - tested to work
    //uploading monitored data to the cloud
    snprintf(req,sizeof(req),
            "GET /update?api_key=%s&field5=%u HTTP/1.1\r\n"
            "Host: %s\r\n"
            "\r\n",
            write_api,
            co2_set,
            host);

    ip_stack.write((unsigned char *)(req),strlen(req));
    vTaskDelay(pdMS_TO_TICKS(2000));
    auto rv = ip_stack.read((unsigned char*)buffer, BUFSIZE, 100);
    if(rv <= 0){
        printf("No response from server\n");
        return false;
    }
    buffer[rv] = 0;
    char* body = extract_thingspeak_http_body();
    if(body){
        int id = atoi(body);
        if(id > 0){
            printf("upload co2 level to network. \n");
            return true;
        }
    }
    printf("upload co2 level to network failed. \n");
    return false;
}*/

//getting co2 set level from talkback queue in cloud. field6 = co2 level set
uint Network::read_co2_set_level(IPStack &ip_stack){
    //ask for command from talkback
    const char *talkback_api = "api_key=WYYFXF0NGSZCUMW6";
    char req[256];
    snprintf(req,sizeof(req), "POST /talkbacks/55392/commands/execute.json HTTP/1.1\r\n"
                      "Host: api.thingspeak.com\r\n"
                      "Content-Length: %d\r\n"
                      "Content-Type: application/x-www-form-urlencoded\r\n"
                      "\r\n"
                      "%s",strlen(talkback_api),talkback_api);

    ip_stack.write((unsigned char *)(req),strlen(req));
    vTaskDelay(pdMS_TO_TICKS(2000));

    auto rv = ip_stack.read((unsigned char*)buffer, BUFSIZE, 100);
    if(rv <= 0){
        printf("No response from server\n");
        //0 for failed readings.
        return 0;
    }
    buffer[rv] = 0;

    //full http response in json format
    printf("HTTP Response: %s\n", buffer);

    //handles json format and retrieve co2 set level
    char* json = strchr(buffer, '{');
    if(!json){
        return 0;
    }
    const char* command = R"("command_string":")";
    char* set_number = strstr(json,command);
    set_number += strlen(command);
    char* end = strchr(set_number, '"');
    if(end){
        *end = '\0';
        return (uint)atoi(set_number);
    }

    return 0;
}

//extract json from thingspeak http respond
char* Network::extract_thingspeak_http_body(){
    char* body = strstr(buffer, "\r\n\r\n");
    if(body){
        body += 4;
        return body;
    }
    return nullptr;
}

int Network::disconnect_to_http(IPStack &ip_stack){
    int rc = ip_stack.disconnect();
    return rc;
}
