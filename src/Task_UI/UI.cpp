#include "UI.h"

#include <cstdio>

UI* UI::instance = nullptr;

UI::UI(QueueHandle_t to_CO2, QueueHandle_t to_Network, QueueHandle_t to_UI,uint32_t stack_size, UBaseType_t priority):
    to_CO2(to_CO2),
    to_Network(to_Network) ,
    to_UI(to_UI),
    rotA(10, true, false, false),
    rotB(11),
    sw(12) {

    // initial monitored data to display before real measurements are there
    received.type = MONITORED_DATA;
    received.data.co2_val = 0;
    received.data.temperature = 0.0;
    received.data.humidity = 0.0;
    received.data.fan_speed = 0;
    received.co2_set = 0;

    //to be used in the isr accessing
    instance = this;

    encoder_queue = xQueueCreate(30, sizeof(encoderEv));
    vQueueAddToRegistry(encoder_queue, "Encoder_Q");

    gpio_set_irq_enabled_with_callback(rotA, GPIO_IRQ_EDGE_FALL, true, &encoder_callback);
    gpio_set_irq_enabled(sw, GPIO_IRQ_EDGE_FALL, true);

    // creating the task
    xTaskCreate(task_wrap, name, stack_size, this, priority, nullptr);
}

void UI::encoder_callback(uint gpio, uint32_t events) {
    if (instance) {
        instance->handleEncoderCallback(gpio, events);
    }
}
// implementation of detectin encoder events
void UI::handleEncoderCallback(uint gpio, uint32_t events) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    encoderEv ev;
    TickType_t current_time = xTaskGetTickCountFromISR();

    if (gpio == rotA) {
        ev = (rotB.read() == 0) ? ROT_L : ROT_R;
        xQueueSendToBackFromISR(encoder_queue, &ev, &xHigherPriorityTaskWoken);
    }

    if (gpio == sw) {
        if ((current_time - last_sw_time) >= pdMS_TO_TICKS(250)) {
            ev = SW;
            xQueueSendToBackFromISR(encoder_queue, &ev, &xHigherPriorityTaskWoken);
            last_sw_time = current_time;
        }
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void UI::task_wrap(void *pvParameters) {
    auto *test = static_cast<UI*>(pvParameters);
    test->task_impl();
}

void UI::task_impl() {
    Message send{};
    encoderEv ev;

    // initializing oled
    i2cbus1 = std::make_shared<PicoI2C>(1, 400000);
    display = std::make_unique<ssd1306os>(i2cbus1);

    //this is just for testing
    co2_set = 1200;
    bool tested = true;

    while (true) {
        bool data_updated = false;
        Message temp_received;
        // non-blocking queue to receive
        if (xQueueReceive(to_UI, &temp_received, 0)) {
            if (temp_received.type == MONITORED_DATA) {
                received.data.co2_val = temp_received.data.co2_val;
                received.data.temperature = temp_received.data.temperature;
                received.data.humidity = temp_received.data.humidity;
                received.data.fan_speed = temp_received.data.fan_speed;
                data_updated = true;
            } else if (received.type == CO2_SET_DATA) {
                co2_set = temp_received.co2_set;
                data_updated = true;
            }
            if (data_updated && current_screen == WELCOME) {
                screen_needs_update = true;
            }
        }

        // encoder event handling
        if (xQueueReceive(encoder_queue, &ev, pdMS_TO_TICKS(10))) {
            if (current_screen == CO2_CHANGE) {
                if (editing_co2) {
                    // encoder functionality when editing co2
                    if (ev == ROT_L) {
                        if (co2_edit < max_co2) {
                            co2_edit += 10;
                            screen_needs_update = true;
                        }
                    }
                    else if (ev == ROT_R) {
                        if (co2_edit > min_co2) {
                            co2_edit -= 10;
                            screen_needs_update = true;
                        }
                    }
                    else if (ev == SW) {
                        // if pressing the button, editing co2 is exited and menu items can be selected
                        editing_co2 = false;
                        current_menu_item = 0;
                        screen_needs_update = true;
                    }
                }
                else {
                    // iterating back/save menu
                    if (ev == ROT_L) {
                        if (current_menu_item < menu_size - 1) {
                            current_menu_item++;
                            screen_needs_update = true;
                        }
                    }
                    else if (ev == ROT_R) {
                        if (current_menu_item > 0) {
                            current_menu_item--;
                            screen_needs_update = true;
                        }
                    }
                    else if (ev == SW) {
                        if (current_menu_item == 0) {
                            co2_set = co2_edit;
                            // when saving the value need to send to queue
                            send.type = CO2_SET_DATA;
                            send.co2_set = co2_set;
                            printf("FROM UI:%u\n", send.co2_set);
                            xQueueSendToBack(to_Network, &send, portMAX_DELAY);
                            xQueueSendToBack(to_CO2, &send, portMAX_DELAY);
                            current_screen = WELCOME;
                            editing_co2 = false;
                        } else {
                            //if back, then welcome screen
                            current_screen = WELCOME;
                            editing_co2 = false;
                        }
                        current_menu_item = 0;
                        screen_needs_update = true;
                    }
                }
            }
            else {
                // in other screens just iterate menu
                if (ev == ROT_L) {
                    if (current_menu_item < menu_size - 1) {
                        current_menu_item++;
                        screen_needs_update = true;
                    }
                }
                else if (ev == ROT_R) {
                    if (current_menu_item > 0) {
                        current_menu_item--;
                        screen_needs_update = true;
                    }
                }
                else if (ev == SW) {
                    switch (current_screen) {
                        case WELCOME:
                            if (current_menu_item == 0) {
                                current_screen = CO2_CHANGE;
                                editing_co2 = true;  // start editing mode
                                co2_edit = co2_set; // display fist current co2_set value
                            } else {
                                current_screen = NET_SET;
                            }
                            current_menu_item = 0;
                            screen_needs_update = true;
                            break;

                        case NET_SET:
                            if (current_menu_item == 0) {
                                current_screen = SSID;
                            } else {
                                current_screen = WELCOME;
                            }
                            current_menu_item = 0;
                            screen_needs_update = true;
                            break;
                    }
                }
            }
        }

        if (screen_needs_update) {
            display_screen();
            screen_needs_update = false;
        }

        //sending CO2 data, needs to be triggered when the user change
        /*now it's test
        if(!tested){
            //queue to network, it needs to send type, co2 set level.
            send.type = CO2_SET_DATA;
            send.co2_set = co2_set;
            xQueueSendToBack(to_Network, &send, portMAX_DELAY);
            printf("QUEUE from UI from network: %d\n", send.co2_set);
            //queue to co2, it only needs to send uint co2 set value.
            xQueueSendToBack(to_CO2, &co2_set, portMAX_DELAY);
            printf("QUEUE from UI to CO2: %d\n", co2_set);
            tested = true;
        }*/
    }
}

// function to display different screens
void UI::display_screen() {
    display->fill(0);

    switch (current_screen) {
        case WELCOME:
            char co2_val[32];
            char co2_target[32];
            char temp[32];
            char hum[32];
            char fan[32];
            snprintf(co2_val, sizeof(co2_val), "co2:  %uppm", received.data.co2_val);
            snprintf(co2_target, sizeof(co2_target), "set:  %uppm", co2_set);
            snprintf(temp, sizeof(temp), "t:    %.1fC", received.data.temperature);
            snprintf(hum, sizeof(hum), "rh:   %.1f/%%", received.data.humidity);
            snprintf(fan, sizeof(fan), "fan:  %u%%", received.data.fan_speed);


            display->text(co2_val, 0, 0);
            display->text(temp, 0, line_height*2);
            display->text(co2_target, 0, line_height);
            display->text(hum, 0, line_height*3);
            display->text(fan, 0, line_height*4);

        // Show menu with selection indicator
        for (uint i = 0; i < 2; i++) {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%c %s",
                (i == current_menu_item) ? '>' : ' ',
                welcome_menu[i]);
            display->text(buffer, 0, line_height * (6 + i));
        }
        break;

        case CO2_CHANGE:
            char co2_buffer[32];
            if (editing_co2) {
                snprintf(co2_buffer, sizeof(co2_buffer), "> %d ppm", co2_edit);
            } else {
                snprintf(co2_buffer, sizeof(co2_buffer), "  %d ppm", co2_edit);
            }
            display->text(co2_buffer, (oled_width/2)-50, line_height*2);

            for (uint i = 0; i < 2; i++) {
                char buffer[32];
                snprintf(buffer, sizeof(buffer), "%c %s",
                    (!editing_co2 && i == current_menu_item) ? '>' : ' ',
                    co2_menu[i]);
                display->text(buffer, 0, line_height * (6 + i));
            }
            break;

        case NET_SET:
            display->text("CURRENT:", (oled_width-100), 0);

            //todo: get network from EEPROM and show variable not hardcode
            display->text("Julijaiph", (oled_width-105), line_height*2);

            for (uint i = 0; i < 2; i++) {
                char buffer[32];
                snprintf(buffer, sizeof(buffer), "%c %s",
                    (i == current_menu_item) ? '>' : ' ',
                    network_menu[i]);
                display->text(buffer, 0, line_height * (5 + i));
            }
            break;
    }

    display->show();
}



