#include "icons.h"
#include "cstring"
#include <cstdio>
#include "SystemContext.h"
#include "hardware/gpio.h"
#include "PicoI2C.h"

using namespace std;

// ============ SCREEN METHODS ============

void currentScreen::welcome() {
    ssd->fill(0);
    ssd->text("GROUP 07", 28, 5);
    ssd->text("Carol", 42, 22);
    ssd->text("Julija", 38, 34);
    ssd->text("Linh", 46, 46);
    ssd->text("Qi", 54, 58);
    ssd->show();
}

void currentScreen::screenSelection() {
    auto& ctx = SystemContext::getInstance();
    color0 = color1 = color2 = 1;

    ssd->fill(0);
    switch (ctx.selection_screen_option) {
        case 0:
            color0 = 0;
            ssd->fill_rect(0, 0, 128, 11, 1);
            break;
        case 1:
            color1 = 0;
            ssd->fill_rect(0, 16, 128, 11, 1);
            break;
        case 2:
            color2 = 0;
            ssd->fill_rect(0, 30, 128, 11, 1);
            break;
        default:
            break;
    }
    ssd->text("SHOW INFO", 28, 2, color0);
    ssd->text("SET CO2 LEVEL", 8, 18, color1);
    ssd->text("CONFIG WIFI", 20, 32, color2);
    ssd->fill_rect(0, 53, 33, 10, 1);
    ssd->text("OK", 1, 54, 0);
    ssd->show();
}

void currentScreen::setCo2(const int val) {
    ssd->fill(0);
    ssd->text("CO2 LEVEL", 50, 20);
    text = to_string(val) + " ppm";
    ssd->text(text, 55, 38);
    ssd->fill_rect(0, 0, 33, 10, 1);
    ssd->text("BACK", 1, 1, 0);
    ssd->fill_rect(0, 53, 33, 10, 1);
    ssd->text("OK", 1, 54, 0);
    ssd->show();
}

void currentScreen::info(const int c, const int t, const int h, const int s, const int sp) {
    ssd->fill(0);
    mono_vlsb icon(info_icon, 20, 20);
    ssd->blit(icon, 0, 40);
    text = "CO2:" + to_string(c) + " ppm";
    ssd->text(text, 38, 2);
    text = "RH:" + to_string(h) + " %";
    ssd->text(text, 38, 15);
    text = "T:" + to_string(t) + " C";
    ssd->text(text, 38, 28);
    text = "S:" + to_string(s) + " %";
    ssd->text(text, 38, 41);
    text = "SP:" + to_string(sp) + " ppm";
    ssd->text(text, 38, 54);
    ssd->fill_rect(0, 0, 33, 10, 1);
    ssd->text("BACK", 1, 1, 0);
    ssd->show();
}

void currentScreen::wifiConfig(const char* ssid, const char* pw) {
    auto& ctx = SystemContext::getInstance();
    ssd->fill(0);
    switch (ctx.wifi_screen_option) {
        case 0:
            ssd->rect(0,17,128,13,1);
            break;
        case 1:
            ssd->rect(0,32,128,13,1);
            break;
        default:
            break;
    }
    ssd->text("SSID:", 2,20);
    ssd->text(ssid, 44, 20);
    ssd->text("PASS:",2,35);
    ssd->text(pw, 44, 35);
    ssd->fill_rect(0, 0, 33, 10, 1);
    ssd->text("BACK", 1, 1, 0);
    ssd->fill_rect(0, 53, 33, 10, 1);
    ssd->text("OK", 1, 54, 0);
    ssd->show();
}

void currentScreen::asciiCharSelection(const int posX, const int asciiChar) {
    auto& ctx = SystemContext::getInstance();
    int posY = 0;
    char c[2];
    sprintf(c, "%c", asciiChar);
    switch (ctx.wifi_screen_option) {
        case 0: posY = 20; break;
        case 1: posY = 35; break;
        default: break;
    }
    if (asciiChar < START_ASCII) {
        ssd->fill_rect(posX,posY,8,8,1);
    } else {
        ssd->fill_rect(posX,posY,8,8,0);
        ssd->text(c, posX, posY);
    }
    ssd->show();
}

// ============ GPIO ISR CALLBACK ============

void hw_ISR_CB(uint gpio, uint32_t events){
    auto& ctx = SystemContext::getInstance();
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
    if (gpio == ctx.ok_btn){
        ctx.rot_btn_data.action_type = GpioAction::OK_PRESS;
        xQueueSendFromISR(ctx.gpio_action_q, &ctx.rot_btn_data.action_type, &pxHigherPriorityTaskWoken);
    }
    if (gpio == ctx.back_btn){
        ctx.rot_btn_data.action_type = GpioAction::BACK_PRESS;
        xQueueSendFromISR(ctx.gpio_action_q, &ctx.rot_btn_data.action_type, &pxHigherPriorityTaskWoken);
    }
    if (gpio == ctx.rotA){
        if (gpio_get(ctx.rotA) != gpio_get(ctx.rotB)) ctx.rot_btn_data.action_type = GpioAction::ROT_CW;
        else ctx.rot_btn_data.action_type = GpioAction::ROT_CCW;
        xQueueSendFromISR(ctx.gpio_action_q, &ctx.rot_btn_data.action_type, &pxHigherPriorityTaskWoken);
    }
    portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
}

// ============ DISPLAY TASK ============

void SystemContext::displayTaskWrapper(void* param) {
    getInstance().displayTask();
}

void SystemContext::displayTask() {
    auto i2cbus{std::make_shared<PicoI2C>(1, 400000)};
    auto display = std::make_shared<ssd1306os>(i2cbus);
    currentScreen screen(display);

    RotBtnData temp_data;
    WifiCharPos char_pos_X;

    // Show welcome screen (team name)
    screen.welcome();
    vTaskDelay(pdMS_TO_TICKS(5000));

    // then show the selection menu
    rot_btn_data.screen_type = ScreenType::SELECTION_SCR;
    screen.screenSelection();

    while(true) {
        // Handle GPIO-driven UI events
        if (xQueueReceive(gpio_data_q, &temp_data, pdMS_TO_TICKS(20))){
            // BACK pressed -> always go to selection screen
            if (temp_data.action_type == GpioAction::BACK_PRESS){
                rot_btn_data.screen_type = ScreenType::SELECTION_SCR;
                screen.screenSelection();
            }
            // OK pressed -> depending on selection, enter appropriate screen or confirm
            else if (temp_data.action_type == GpioAction::OK_PRESS){
                if (rot_btn_data.screen_type == ScreenType::SELECTION_SCR) {
                    // Enter chosen screen
                    if (selection_screen_option == 0){        // Info
                        rot_btn_data.screen_type = ScreenType::INFO_SCR;
                        screen.info(co2, temp, rh, speed, setpoint);
                    } else if (selection_screen_option == 1){ // Set CO2
                        rot_btn_data.screen_type = ScreenType::SET_CO2_SCR;
                        setpoint_temp = setpoint;
                        screen.setCo2(setpoint_temp);
                    } else if (selection_screen_option == 2){ // WiFi config
                        rot_btn_data.screen_type = ScreenType::WIFI_CONF_SCR;
                        screen.wifiConfig(SSID_WIFI, PASS_WIFI);
                    }
                } else if (rot_btn_data.screen_type == ScreenType::SET_CO2_SCR){
                    // confirm new setpoint
                    setpoint = setpoint_temp;
                    xEventGroupSetBits(co2EventGroup, CO2_CHANGE_BIT_EEPROM);
                    // stay in SET_CO2 screen instead of going to INFO
                    screen.setCo2(setpoint_temp);
                } else if (rot_btn_data.screen_type == ScreenType::WIFI_CONF_SCR){
                    // Wi-Fi editing handled by gpio_task state machine; display simply refreshes
                    screen.wifiConfig(SSID_WIFI, PASS_WIFI);
                } else if (rot_btn_data.screen_type == ScreenType::INFO_SCR) {
                    // nothing special for OK in info screen; keep showing info
                    screen.info(co2, temp, rh, speed, setpoint);
                }
            }
            // Rotation events -> refresh appropriate screen
            else if (temp_data.action_type == GpioAction::ROT_CW || temp_data.action_type == GpioAction::ROT_CCW){
                if (rot_btn_data.screen_type == ScreenType::SELECTION_SCR){
                    // show menu highlight updated from gpio_task (selection_screen_option updated there)
                    screen.screenSelection();
                } else if (rot_btn_data.screen_type == ScreenType::SET_CO2_SCR){
                    screen.setCo2(setpoint_temp);
                } else if (rot_btn_data.screen_type == ScreenType::WIFI_CONF_SCR){
                    screen.wifiConfig(SSID_WIFI, PASS_WIFI);
                } else if (rot_btn_data.screen_type == ScreenType::INFO_SCR){
                    screen.info(co2, temp, rh, speed, setpoint);
                }
            }
        }

        // WiFi character selection stream (when editing)
        if (xQueueReceive(wifiCharPos_q, &char_pos_X, pdMS_TO_TICKS(20))){
            screen.asciiCharSelection(char_pos_X.pos_x, char_pos_X.ch);
        }

        // If CO2 changed, refresh info screen (only if in info screen)
        EventBits_t bits = xEventGroupWaitBits(co2EventGroup,
                                              CO2_CHANGE_BIT_LCD,
                                              pdTRUE, pdFALSE, pdMS_TO_TICKS(20));
        if (bits & CO2_CHANGE_BIT_LCD){
            if (rot_btn_data.screen_type == ScreenType::INFO_SCR){
                screen.info(co2, temp, rh, speed, setpoint);
            }
        }
    }
}

// ============ GPIO TASK ============

void SystemContext::gpioTaskWrapper(void* param) {
    getInstance().gpioTask();
}

void SystemContext::gpioTask() {
    // Initialize GPIOs
    gpio_init(rotA);
    gpio_set_dir(rotA, GPIO_IN);
    gpio_pull_up(rotA);

    gpio_init(rotB);
    gpio_set_dir(rotB, GPIO_IN);
    gpio_pull_up(rotB);

    gpio_init(led1);
    gpio_set_dir(led1, GPIO_OUT);

    gpio_init(valve);
    gpio_set_dir(valve, GPIO_OUT);

    gpio_init(ok_btn);
    gpio_set_dir(ok_btn, GPIO_IN);
    gpio_pull_up(ok_btn);

    gpio_init(back_btn);
    gpio_set_dir(back_btn, GPIO_IN);
    gpio_pull_up(back_btn);

    // Set up interrupts
    gpio_set_irq_enabled_with_callback(rotA, GPIO_IRQ_EDGE_FALL, true, &hw_ISR_CB);
    gpio_set_irq_enabled_with_callback(ok_btn, GPIO_IRQ_EDGE_FALL, true, &hw_ISR_CB);
    gpio_set_irq_enabled_with_callback(back_btn, GPIO_IRQ_EDGE_FALL, true, &hw_ISR_CB);

    // Wi-Fi edit helpers
    char backup_ssid[50] = "\0";
    char backup_pass[50] = "\0";
    int posX = SSID_POS_X;
    int asciiChar = START_ASCII;
    uint64_t lastPressTime = time_us_64();
    strcpy(backup_ssid, SSID_WIFI);
    strcpy(backup_pass, PASS_WIFI);

    while(true) {
        if (xQueueReceive(gpio_action_q, &rot_btn_data.action_type, portMAX_DELAY)){
            switch (rot_btn_data.action_type){

                // ---------------- BACK PRESS ----------------
                case GpioAction::BACK_PRESS:
                    rot_btn_data.screen_type = ScreenType::SELECTION_SCR;
                    setpoint_temp = setpoint;
                    isEditing = false;
                    xQueueSend(gpio_data_q, &rot_btn_data, 0);
                    break;

                // ---------------- OK PRESS ----------------
                case GpioAction::OK_PRESS:
                    gpio_set_irq_enabled_with_callback(ok_btn, GPIO_IRQ_EDGE_FALL, false, &hw_ISR_CB);
                    if (time_us_64() - lastPressTime > 30000){ // debounce 30ms
                        lastPressTime = time_us_64();
                        if (rot_btn_data.screen_type == ScreenType::SELECTION_SCR){
                            if (selection_screen_option == 0){
                                rot_btn_data.screen_type = ScreenType::INFO_SCR;
                                xQueueSend(gpio_data_q, &rot_btn_data, 0);
                            } else if (selection_screen_option == 1){
                                rot_btn_data.screen_type = ScreenType::SET_CO2_SCR;
                                setpoint_temp = setpoint;
                                xQueueSend(gpio_data_q, &rot_btn_data, 0);
                            } else if (selection_screen_option == 2){
                                rot_btn_data.screen_type = ScreenType::WIFI_CONF_SCR;
                                xQueueSend(gpio_data_q, &rot_btn_data, 0);
                            }
                        } else if (rot_btn_data.screen_type == ScreenType::SET_CO2_SCR){
                            setpoint = setpoint_temp;
                            xEventGroupSetBits(co2EventGroup, CO2_CHANGE_BIT_EEPROM);
                        } else if (rot_btn_data.screen_type == ScreenType::WIFI_CONF_SCR){
                            if (!isEditing) {
                                // handle long/short press enter editing
                                int count = 0;
                                while (!gpio_get(ok_btn) && ++count < 150){
                                    vTaskDelay(pdMS_TO_TICKS(10));
                                }
                                if (count >= 150){ // long press to enter editing
                                    isEditing = true;
                                    switch (wifi_screen_option) {
                                        case 0:
                                            strcpy(SSID_WIFI, "");
                                            strcpy(backup_ssid, "");
                                            posX = SSID_POS_X;
                                            break;
                                        case 1:
                                            strcpy(PASS_WIFI, "");
                                            strcpy(backup_pass, "");
                                            posX = PASS_POS_X;
                                            break;
                                    }
                                    asciiChar = START_ASCII - 1;
                                    wifi_char_pos_X.pos_x = posX;
                                    wifi_char_pos_X.ch = asciiChar;
                                    xQueueSend(gpio_data_q, &rot_btn_data, 0);
                                    xQueueSend(wifiCharPos_q, &wifi_char_pos_X, 0);
                                } else { // short press -> save data
                                    printf("saving data...\n");
                                    xEventGroupSetBits(wifiEventGroup, WIFI_CHANGE_BIT_EEPROM);
                                }
                            } else {
                                // editing characters
                                int count = 0;
                                while (!gpio_get(ok_btn) && ++count < 150){
                                    vTaskDelay(pdMS_TO_TICKS(10));
                                }
                                if (count >= 150){ // long press exit editing
                                    isEditing = false;
                                    if (wifi_screen_option == 0){
                                        strcpy(SSID_WIFI, backup_ssid);
                                        SSID_WIFI[strlen(SSID_WIFI)] = '\0';
                                    } else {
                                        strcpy(PASS_WIFI, backup_pass);
                                        PASS_WIFI[strlen(PASS_WIFI)] = '\0';
                                    }
                                    xQueueSend(gpio_data_q, &rot_btn_data, 0);
                                } else { // short press confirm char
                                    char c[2];
                                    sprintf(c, "%c", asciiChar);
                                    if (wifi_screen_option == 0){
                                        strcat(SSID_WIFI, c);
                                        strcat(backup_ssid, c);
                                    } else {
                                        strcat(PASS_WIFI, c);
                                        strcat(backup_pass, c);
                                    }
                                    vTaskDelay(pdMS_TO_TICKS(5));
                                    posX += CHAR_WIDTH;
                                    if (posX > MAX_POS_X){
                                        posX -= CHAR_WIDTH;
                                        // scroll back 1 char
                                        if (wifi_screen_option == 0){
                                            for (int i = 0; i < strlen(SSID_WIFI); i++){
                                                SSID_WIFI[i] = SSID_WIFI[i+1];
                                            }
                                        } else {
                                            for (int i = 0; i < strlen(PASS_WIFI); i++){
                                                PASS_WIFI[i] = PASS_WIFI[i+1];
                                            }
                                        }
                                        wifi_char_pos_X.pos_x = posX;
                                        wifi_char_pos_X.ch = asciiChar;
                                        xQueueSend(gpio_data_q, &rot_btn_data, 0);
                                        xQueueSend(wifiCharPos_q, &wifi_char_pos_X, 0);
                                    }
                                    asciiChar = START_ASCII - 1;
                                    wifi_char_pos_X.pos_x = posX;
                                    wifi_char_pos_X.ch = asciiChar;
                                    xQueueSend(wifiCharPos_q, &wifi_char_pos_X, 0);
                                }
                            }
                        }
                    }
                    while (!gpio_get(ok_btn)) vTaskDelay(pdMS_TO_TICKS(10));
                    gpio_set_irq_enabled_with_callback(ok_btn, GPIO_IRQ_EDGE_FALL, true, &hw_ISR_CB);
                    break;

                // ---------------- ROTATE CW ----------------
                case GpioAction::ROT_CW:
                    if (rot_btn_data.screen_type == ScreenType::SELECTION_SCR){
                        selection_screen_option = (selection_screen_option + 1) % 3;
                        xQueueSend(gpio_data_q, &rot_btn_data, 0);
                    } else if (rot_btn_data.screen_type == ScreenType::SET_CO2_SCR){
                        setpoint_temp = (setpoint_temp < MAX_SETPOINT) ?
                                             setpoint_temp + 10 : MAX_SETPOINT;
                        xQueueSend(gpio_data_q, &rot_btn_data, 0);
                    } else if (rot_btn_data.screen_type == ScreenType::WIFI_CONF_SCR){
                        if (!isEditing){
                            wifi_screen_option = !wifi_screen_option;
                            xQueueSend(gpio_data_q, &rot_btn_data, 0);
                        } else {
                            if (++asciiChar > END_ASCII) asciiChar = START_ASCII;
                            wifi_char_pos_X.pos_x = posX;
                            wifi_char_pos_X.ch = asciiChar;
                            xQueueSend(wifiCharPos_q, &wifi_char_pos_X, 0);
                        }
                    }
                    break;

                // ---------------- ROTATE CCW ----------------
                case GpioAction::ROT_CCW:
                    if (rot_btn_data.screen_type == ScreenType::SELECTION_SCR){
                        selection_screen_option = (selection_screen_option + 2) % 3;
                        xQueueSend(gpio_data_q, &rot_btn_data, 0);
                    } else if (rot_btn_data.screen_type == ScreenType::SET_CO2_SCR){
                        setpoint_temp = (setpoint_temp > MIN_SETPOINT) ?
                                             setpoint_temp - 10 : MIN_SETPOINT;
                        xQueueSend(gpio_data_q, &rot_btn_data, 0);
                    } else if (rot_btn_data.screen_type == ScreenType::WIFI_CONF_SCR){
                        if (!isEditing){
                            wifi_screen_option = !wifi_screen_option;
                            xQueueSend(gpio_data_q, &rot_btn_data, 0);
                        } else {
                            if (--asciiChar < START_ASCII) asciiChar = END_ASCII;
                            wifi_char_pos_X.pos_x = posX;
                            wifi_char_pos_X.ch = asciiChar;
                            xQueueSend(wifiCharPos_q, &wifi_char_pos_X, 0);
                        }
                    }
                    break;

                default: break;
            }
        }
    }
}
