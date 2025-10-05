#include "Network.h"

#include <cstdio>
#include <cstring>

// tried to connect with loading credentials to statics. Didn't work. Only string literals/#define works
static char wifi_ssid[32];
static char wifi_password[64];

Network::Network(QueueHandle_t to_CO2,  QueueHandle_t to_UI, QueueHandle_t to_Network,uint32_t stack_size, UBaseType_t priority):
    to_CO2(to_CO2),to_UI (to_UI),to_Network(to_Network){

    load_wifi_cred();
    xTaskCreate(task_wrap, name, stack_size, this, priority, nullptr);
}

void Network::task_wrap(void *pvParameters) {
    auto *test = static_cast<Network*>(pvParameters);
    test->task_impl();
}

void Network::task_impl() {
    //load_wifi_cred();

    Message received{};
    Message send{};

    //test data for thingspeak!!!
    uint co2_set = 1400;
    bool tested_upload = false;
    bool tested = true;
    //this is a test, build the monitored data test numbers
    Monitored_data monitored_data{};
    monitored_data.co2_val = 1300;
    monitored_data.temperature = 25;
    monitored_data.humidity = 55;

    received.data = monitored_data;
    received.type = MONITORED_DATA;

    printf("tries to connect");

    //printf("SSID: %s, PASSWORD: %s\n", SSID, PASSWORD);
    IPStack ip_stack;

    bool cloud_connected = connect_to_cloud(ip_stack,wifi_ssid,wifi_password);

    printf("wifi: %d. http: %d\n",wifi_connected,http_connected);

    //Main logic to receive data from the queue, DO NOT DELETE! WE NEED IT.
    while (true) {
        //get data from CO2_control and UI. data type received: 1. monitored data. 2. uint CO2 set level.
        if (xQueueReceive(to_Network, &received, portMAX_DELAY)) {
            //the received data is from CO2_control_task
            if(received.type == MONITORED_DATA){
                printf("QUEUE to Network from CO2_control_task: co2: %d\n", received.data.co2_val);
                printf("thingspeak: temp: %.1f\n", received.data.temperature);
                monitored_data.co2_val = received.data.co2_val;
                monitored_data.temperature = received.data.temperature;
                monitored_data.humidity = received.data.humidity;
                monitored_data.fan_speed = received.data.fan_speed;
            }
            printf("trying to connect to http");
            if(connect_to_http(ip_stack)){
                printf("goes here\n");
                printf("Connected\n");
                bool ok = upload_sensor_data(ip_stack,monitored_data);
                disconnect_to_http(ip_stack);
                if(ok){
                    printf("Upload success.\n");
                }else{
                    printf("Failed to upload.\n");
                }
            }else{
                printf("Connection failed\n");
            }

        }
        //the received data is from UI task
        else if(received.type == CO2_SET_DATA){
            co2_set = received.co2_set;
            // not sure if this is ok?
            upload_co2_set_level(ip_stack, co2_set);
            //printf("QUEUE to network from UI: co2_set: %d\n", co2_set);
        }

        if(tested){
            //queue to UI, it needs to send type, co2 set level.
            send.type = CO2_SET_DATA;
            send.co2_set = co2_set;
            xQueueSendToBack(to_UI, &co2_set, portMAX_DELAY);
            //printf("QUEUE from network from UI: co2_set: %d\n", co2_set);
            //queue to co2, it only needs to send uint co2 set value.
            xQueueSendToBack(to_CO2, &co2_set, portMAX_DELAY);
            printf("QUEUE from network to co2: co2_set: %d\n", co2_set);
            tested = true;
        }

        //test only once
        //if(!tested_upload){

            //tested_upload = true;
        //}

        //check every 15 from talkback queue
        if (connect_to_http(ip_stack)) {
            uint co2_set = read_co2_set_level(ip_stack);
            if(co2_set>0){
                upload_co2_set_level(ip_stack,co2_set);
            };
            disconnect_to_http(ip_stack);
            vTaskDelay(pdMS_TO_TICKS(15000));
        }

    }

    }



void Network::load_wifi_cred() {
    // Set defaults
    strncpy(wifi_ssid, "Julijaiph", sizeof(wifi_ssid));
    strncpy(wifi_password, "12341234", sizeof(wifi_password));

    // Read from EEPROM
    // eeprom_read(...);

    wifi_ssid[sizeof(wifi_ssid) - 1] = '\0';
    wifi_password[sizeof(wifi_password) - 1] = '\0';
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
            disconnect_to_http(ip_stack);
        }
    }

    return (wifi_connected && http_connected);
}

//upload monitored data to the sensor
bool Network::upload_sensor_data(IPStack &ip_stack, Monitored_data &data){
    char req[256];

    // Update fields using a minimal GET request - tested to work
    //uploading monitored data to the cloud
    snprintf(req,sizeof(req),
            "GET /update?api_key=%s&field1=%u&field2=%.2f&field3=%.2f&field4=%u HTTP/1.1\r\n"
            "Host: %s\r\n"
            "\r\n",
            write_api,
            data.co2_val,
            data.temperature,
            data.humidity,
            data.fan_speed,
            host);

    ip_stack.write((unsigned char *)(req),strlen(req),1000);
    auto rv = ip_stack.read((unsigned char*)buffer, BUFSIZE, 2000);
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
bool Network::upload_co2_set_level(IPStack &ip_stack, uint co2_set){
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

    ip_stack.write((unsigned char *)(req),strlen(req),1000);
    auto rv = ip_stack.read((unsigned char*)buffer, BUFSIZE, 2000);
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
}

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

    ip_stack.write((unsigned char *)(req),strlen(req),1000);

    auto rv = ip_stack.read((unsigned char*)buffer, BUFSIZE, 2000);
    if(rv <= 0){
        printf("No response from server\n");
        //negative numbers or 0 for failed readings.
        return static_cast<uint>(rv);
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
