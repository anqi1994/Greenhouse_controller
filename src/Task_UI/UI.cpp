#include "UI.h"

#include <cstdio>

UI* UI::instance = nullptr;

UI::UI(QueueHandle_t to_CO2, QueueHandle_t to_Network, QueueHandle_t to_UI, EventGroupHandle_t network_event_group, uint32_t stack_size, UBaseType_t priority):
    to_CO2(to_CO2),
    to_Network(to_Network) ,
    to_UI(to_UI),
    network_event_group(network_event_group),
    rotA(10, true, false, false),
    rotB(11),
    sw(12),
    done_button(8)
    {

    // initial monitored data to display before real measurements are there
    received.type = MONITORED_DATA;
    received.data.co2_val = 0;
    received.data.temperature = 0.0;
    received.data.humidity = 0.0;
    received.data.fan_speed = 0;
    received.co2_set = 0;

    //to be used in the isr accessing
    instance = this;

    encoder_queue = xQueueCreate(10, sizeof(encoderEv));
    vQueueAddToRegistry(encoder_queue, "Encoder_Q");

    gpio_set_irq_enabled_with_callback(rotA, GPIO_IRQ_EDGE_FALL, true, &encoder_callback);
    gpio_set_irq_enabled(sw, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(done_button, GPIO_IRQ_EDGE_FALL, true);

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

    if (gpio == done_button) {
        if ((current_time - last_done_time) >= pdMS_TO_TICKS(250)) {
            ev = DONE;
            xQueueSendToBackFromISR(encoder_queue, &ev, &xHigherPriorityTaskWoken);
            last_done_time = current_time;
        }
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void UI::task_wrap(void *pvParameters) {
    auto *test = static_cast<UI*>(pvParameters);
    test->task_impl();
}

void UI::task_impl() {
    //Message send{};
    encoderEv ev;

    // initializing oled
    i2cbus1 = std::make_shared<PicoI2C>(1, 400000);
    display = std::make_unique<ssd1306os>(i2cbus1);

    //this is just for testing
    co2_set = 0;
    //bool tested = true;

    while (true) {
        //bool data_updated = false;
        Message temp_received;
        // non-blocking queue to receive from control/netw
        if (xQueueReceive(to_UI, &temp_received, 0)) {
            if (temp_received.type == MONITORED_DATA) {
                received.data = temp_received.data;
                if (current_screen == WELCOME) {
                    screen_needs_update = true;
                }
            } else if (temp_received.type == CO2_SET_DATA) {
                co2_set = temp_received.co2_set;
                if (current_screen == WELCOME) {
                    screen_needs_update = true;
                }
            }
        }

        //display updated when something changes
        if (screen_needs_update) {
            display_screen();
            screen_needs_update = false;
        }

        // encoder event handling
        if (xQueueReceive(encoder_queue, &ev, pdMS_TO_TICKS(10))) {
            //switch statement for calling screen handling dunctions
            switch (current_screen) {
                case WELCOME:
                    welcome_screen(ev);
                    break;
                case CO2_CHANGE:
                    co2_screen(ev);
                    break;
                case NET_SET:
                    network_screen(ev);
                    break;
                case SSID:
                    text_entry_screen(ev, ssid_input, MAX_SSID_LENGTH, PASS);
                    break;
                case PASS:
                    text_entry_screen(ev, pass_input, MAX_PASSWORD_LENGTH, WELCOME);
                    break;
            }
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

//turning encoder iterates through possible menu options
void UI::navigate_menu(encoderEv ev) {
    if (ev == ROT_L && current_menu_item < 1) {
        ++current_menu_item;
        screen_needs_update = true;
    } else if (ev == ROT_R && current_menu_item > 0) {
        --current_menu_item;
        screen_needs_update = true;
    }
}

// just setting new screen and marker at first menu item
void UI::change_screen(screens new_screen) {
    current_screen = new_screen;
    current_menu_item = 0;
    screen_needs_update = true;
}

// handling welcome screen and its menu
void UI::welcome_screen(encoderEv ev) {
    if (ev == SW) {
        if (current_menu_item == 0) {
            change_screen(CO2_CHANGE);
            editing_co2 = true;
            co2_edit = co2_set;
        } else {
            change_screen(NET_SET);
        }
    } else {
        navigate_menu(ev);
    }
}

//handling co2 change screen
void UI::co2_screen(encoderEv ev) {
    //value edit mode
    if (editing_co2) {
        if (ev == ROT_L && co2_edit < max_co2) {
            co2_edit += 10;
            screen_needs_update = true;
        } else if (ev == ROT_R && co2_edit > min_co2) {
            co2_edit -= 10;
            screen_needs_update = true;
        } else if (ev == SW) { // exit editing and go to menu
            editing_co2 = false;
            current_menu_item = 0;
            screen_needs_update = true;
        }
    } else {
        if (ev == SW) {
            if (current_menu_item == 0) {
                // save new co2 value and send to queues
                co2_set = co2_edit;
                Message send;
                send.type = CO2_SET_DATA;
                send.co2_set = co2_set;
                printf("FROM UI:%u\n", send.co2_set);
                xQueueSendToBack(to_Network, &send, portMAX_DELAY);
                xQueueSendToBack(to_CO2, &send, portMAX_DELAY);
            }
            change_screen(WELCOME);
            editing_co2 = false;
        } else {
            navigate_menu(ev);
        }
    }
}

// network setting screen
void UI::network_screen(encoderEv ev) {
    if (ev == SW) {
        if (current_menu_item == 0) {
            change_screen(SSID);
            reset_text_entry();
            ssid_input.clear();
        } else {
            change_screen(WELCOME);
        }
    } else {
        navigate_menu(ev);
    }
}

// handling wifi setting changing screens
void UI::text_entry_screen(encoderEv ev, std::string& input_str, uint max_len, screens next_screen) {
    if (selecting_char) {
        //char selection more
        if (ev == ROT_L) {
            current_char_id = (current_char_id + 1) % ascii_length;
            screen_needs_update = true;
        } else if (ev == ROT_R) {
            current_char_id = (current_char_id == 0) ? ascii_length - 1 : current_char_id - 1;
            screen_needs_update = true;
        } else if (ev == SW) {
            // short pressing for adding char
            if (input_str.length() < max_len) {
                input_str += ascii_chars[current_char_id];
                screen_needs_update = true;
            }
        } else if (ev == DONE) {
            // long press to exit entering and go to menu
            if (!input_str.empty()) {
                selecting_char = false;
                current_menu_item = 0;
                screen_needs_update = true;
            }
        }
    } else {
        // menu for deleting and saving
        if (ev == ROT_L && current_menu_item < 1) {
            current_menu_item++;
            screen_needs_update = true;
        } else if (ev == ROT_R && current_menu_item > 0) {
            current_menu_item--;
            screen_needs_update = true;
        } else if (ev == SW) {
            if (current_menu_item == 0) {
                // deleting last char
                if (!input_str.empty()) {
                    input_str.pop_back();
                    screen_needs_update = true;
                }
                // back to selection after deleting
                selecting_char = true;
                current_char_id = 0;
            } else {
                // saving entered credentials
                if (next_screen == PASS) {
                    change_screen(PASS);
                    reset_text_entry();
                    pass_input.clear();
                } else {
                    Message send{};
                    send.type = NETWORK_CONFIG;
                    printf("SSID ENTERED: %s\n", ssid_input.c_str());
                    printf("PASS ENTERED: %s\n", pass_input.c_str());

                    send.network_config.password = pass_input.c_str();
                    send.network_config.ssid = ssid_input.c_str();
                    xQueueSendToBack(to_Network, &send, portMAX_DELAY);
                    change_screen(WELCOME);
                }
            }
        }
    }
}

void UI::reset_text_entry() {
    selecting_char = true;
    current_char_id = 0;
    current_menu_item = 0;
}

// function to display different screen text/menu selections
void UI::display_screen() {
    display->fill(0);
    char buffer[64];

    switch (current_screen) {
        case WELCOME:
            snprintf(buffer, sizeof(buffer), "co2:  %uppm", received.data.co2_val);
            display->text(buffer, 0, 0);

            snprintf(buffer, sizeof(buffer), "t:    %.1fC", received.data.temperature);
            display->text(buffer, 0, line_height);

            snprintf(buffer, sizeof(buffer), "set:  %uppm", co2_set);
            display->text(buffer, 0, line_height*2);

            snprintf(buffer, sizeof(buffer), "rh:   %.1f%%", received.data.humidity);
            display->text(buffer, 0, line_height*3);

            snprintf(buffer, sizeof(buffer), "fan:  %u%%", received.data.fan_speed);
            display->text(buffer, 0, line_height*4);

            for (uint i = 0; i < 2; i++) {
                snprintf(buffer, sizeof(buffer), "%c %s",
                    (i == current_menu_item) ? '>' : ' ',
                    welcome_menu[i]);
                display->text(buffer, 0, line_height * (6 + i));
            }
            break;

        case CO2_CHANGE:
            if (editing_co2) {
                snprintf(buffer, sizeof(buffer), "> %d ppm", co2_edit);
            } else {
                snprintf(buffer, sizeof(buffer), "  %d ppm", co2_edit);
            }
            display->text(buffer, (oled_width/2)-50, line_height*2);

            for (uint i = 0; i < 2; i++) {
                snprintf(buffer, sizeof(buffer), "%c %s",
                    (!editing_co2 && i == current_menu_item) ? '>' : ' ',
                    co2_menu[i]);
                display->text(buffer, 0, line_height * (6 + i));
            }
            break;

        case NET_SET:
            display->text("CURRENT:", (oled_width-100), 0);
            //todo: display variable from queue (from EEPROM)
            display->text("Julijaiph", (oled_width-105), line_height*2);

            for (uint i = 0; i < 2; i++) {
                snprintf(buffer, sizeof(buffer), "%c %s",
                    (i == current_menu_item) ? '>' : ' ',
                    network_menu[i]);
                display->text(buffer, 0, line_height * (5 + i));
            }
            break;

        case SSID:
            display->text("SSID:", 0, 0);

            if (!ssid_input.empty()) {
                display->text(ssid_input.c_str(), 0, line_height*2);
            }

            if (selecting_char) {
                snprintf(buffer, sizeof(buffer), "> %c <", ascii_chars[current_char_id]);
                display->text(buffer, (oled_width/2) - 15, line_height * 3);
                display->text("*SW1 = finish*", 0, line_height*5);
            } else {
                for (uint i = 0; i < 2; i++) {
                    snprintf(buffer, sizeof(buffer), "%c %s",
                        (i == current_menu_item) ? '>' : ' ',
                        text_entry_menu[i]);
                    display->text(buffer, 0, line_height * (6 + i));
                }
            }
            break;

        case PASS:
            display->text("PASSWORD:", 0, 0);

            if (!pass_input.empty()) {
                display->text(pass_input.c_str(), 0, line_height * 2);
            }
            if (selecting_char) {
                snprintf(buffer, sizeof(buffer), "> %c <", ascii_chars[current_char_id]);
                display->text(buffer, (oled_width/2) - 15, line_height * 3);
                display->text("*SW1 = finish*", 0, line_height*5);
            } else {
                for (uint i = 0; i < 2; i++) {
                    snprintf(buffer, sizeof(buffer), "%c %s",
                        (i == current_menu_item) ? '>' : ' ',
                        text_entry_menu[i]);
                    display->text(buffer, 0, line_height * (6 + i));
                }
            }
            break;
    }

    display->show();
}


