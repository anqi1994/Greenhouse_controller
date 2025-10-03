#include <iostream>
#include <sstream>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "hardware/gpio.h"
#include "PicoOsUart.h"
#include "ssd1306os.h"
#include "pico/cyw43_arch.h"
#include "timers.h"
#include "hardware/timer.h"
#include <cstdio>

#include "PicoI2C.h"
#include "blinker.h"
#include "event_groups.h"
#include "SystemContext.h"
#include "screen_selection.h"

#include "ModbusClient.h"
#include "ModbusRegister.h"

extern "C" {
    uint32_t read_runtime_ctr(void) {
        return timer_hw->timerawl;
    }
}

// ---------------- DISPLAY TASK -----------------
void display_task(void *param){
    auto& ctx = SystemContext::getInstance();
    auto i2cbus{std::make_shared<PicoI2C>(1, 400000)};
    auto display = std::make_shared<ssd1306os>(i2cbus);
    currentScreen screen(display);

    RotBtnData temp_data;
    WifiCharPos char_pos_X;

    // Show welcome screen (team name)
    screen.welcome();
    vTaskDelay(pdMS_TO_TICKS(5000));

    // then show the selection menu
    ctx.rot_btn_data.screen_type = ScreenType::SELECTION_SCR;
    screen.screenSelection();

    while(true) {
        // Handle GPIO-driven UI events
        if (xQueueReceive(ctx.gpio_data_q, &temp_data, pdMS_TO_TICKS(20))){
            // BACK pressed -> always go to selection screen
            if (temp_data.action_type == GpioAction::BACK_PRESS){
                ctx.rot_btn_data.screen_type = ScreenType::SELECTION_SCR;
                screen.screenSelection();
            }
            // OK pressed -> depending on selection, enter appropriate screen or confirm
            else if (temp_data.action_type == GpioAction::OK_PRESS){
                if (ctx.rot_btn_data.screen_type == ScreenType::SELECTION_SCR) {
                    // Enter chosen screen
                    if (ctx.selection_screen_option == 0){        // Info
                        ctx.rot_btn_data.screen_type = ScreenType::INFO_SCR;
                        screen.info(ctx.co2, ctx.temp, ctx.rh, ctx.speed, ctx.setpoint);
                    } else if (ctx.selection_screen_option == 1){ // Set CO2
                        ctx.rot_btn_data.screen_type = ScreenType::SET_CO2_SCR;
                        ctx.setpoint_temp = ctx.setpoint;
                        screen.setCo2(ctx.setpoint_temp);
                    } else if (ctx.selection_screen_option == 2){ // WiFi config
                        ctx.rot_btn_data.screen_type = ScreenType::WIFI_CONF_SCR;
                        screen.wifiConfig(ctx.SSID_WIFI, ctx.PASS_WIFI);
                    }
                } else if (ctx.rot_btn_data.screen_type == ScreenType::SET_CO2_SCR){
                    // confirm new setpoint
                    ctx.setpoint = ctx.setpoint_temp;
                    xEventGroupSetBits(ctx.co2EventGroup, SystemContext::CO2_CHANGE_BIT_EEPROM);
                    // stay in SET_CO2 screen instead of going to INFO
                    screen.setCo2(ctx.setpoint_temp);
                } else if (ctx.rot_btn_data.screen_type == ScreenType::WIFI_CONF_SCR){
                    // Wi-Fi editing handled by gpio_task state machine; display simply refreshes
                    screen.wifiConfig(ctx.SSID_WIFI, ctx.PASS_WIFI);
                } else if (ctx.rot_btn_data.screen_type == ScreenType::INFO_SCR) {
                    // nothing special for OK in info screen; keep showing info
                    screen.info(ctx.co2, ctx.temp, ctx.rh, ctx.speed, ctx.setpoint);
                }
            }
            // Rotation events -> refresh appropriate screen
            else if (temp_data.action_type == GpioAction::ROT_CW || temp_data.action_type == GpioAction::ROT_CCW){
                if (ctx.rot_btn_data.screen_type == ScreenType::SELECTION_SCR){
                    // show menu highlight updated from gpio_task (selection_screen_option updated there)
                    screen.screenSelection();
                } else if (ctx.rot_btn_data.screen_type == ScreenType::SET_CO2_SCR){
                    screen.setCo2(ctx.setpoint_temp);
                } else if (ctx.rot_btn_data.screen_type == ScreenType::WIFI_CONF_SCR){
                    screen.wifiConfig(ctx.SSID_WIFI, ctx.PASS_WIFI);
                } else if (ctx.rot_btn_data.screen_type == ScreenType::INFO_SCR){
                    screen.info(ctx.co2, ctx.temp, ctx.rh, ctx.speed, ctx.setpoint);
                }
            }
        }

        // WiFi character selection stream (when editing)
        if (xQueueReceive(ctx.wifiCharPos_q, &char_pos_X, pdMS_TO_TICKS(20))){
            screen.asciiCharSelection(char_pos_X.pos_x, char_pos_X.ch);
        }

        // If CO2 changed, refresh info screen (only if in info screen)
        EventBits_t bits = xEventGroupWaitBits(ctx.co2EventGroup,
                                              SystemContext::CO2_CHANGE_BIT_LCD,
                                              pdTRUE, pdFALSE, pdMS_TO_TICKS(20));
        if (bits & SystemContext::CO2_CHANGE_BIT_LCD){
            if (ctx.rot_btn_data.screen_type == ScreenType::INFO_SCR){
                screen.info(ctx.co2, ctx.temp, ctx.rh, ctx.speed, ctx.setpoint);
            }
        }
    }
}


// ---------------- GPIO ISR CALLBACK -----------------
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

// ---------------- GPIO TASK -----------------
void gpio_task(void *param) {
    auto& ctx = SystemContext::getInstance();
    (void) param;

    // Initialize GPIOs
    gpio_init(ctx.rotA);
    gpio_set_dir(ctx.rotA, GPIO_IN);
    gpio_pull_up(ctx.rotA);

    gpio_init(ctx.rotB);
    gpio_set_dir(ctx.rotB, GPIO_IN);
    gpio_pull_up(ctx.rotB);

    gpio_init(ctx.led1);
    gpio_set_dir(ctx.led1, GPIO_OUT);

    gpio_init(ctx.valve);
    gpio_set_dir(ctx.valve, GPIO_OUT);

    gpio_init(ctx.ok_btn);
    gpio_set_dir(ctx.ok_btn, GPIO_IN);
    gpio_pull_up(ctx.ok_btn);

    gpio_init(ctx.back_btn);
    gpio_set_dir(ctx.back_btn, GPIO_IN);
    gpio_pull_up(ctx.back_btn);

    // Set up interrupts
    gpio_set_irq_enabled_with_callback(ctx.rotA, GPIO_IRQ_EDGE_FALL, true, &hw_ISR_CB);
    gpio_set_irq_enabled_with_callback(ctx.ok_btn, GPIO_IRQ_EDGE_FALL, true, &hw_ISR_CB);
    gpio_set_irq_enabled_with_callback(ctx.back_btn, GPIO_IRQ_EDGE_FALL, true, &hw_ISR_CB);

    // Wi-Fi edit helpers
    char backup_ssid[50] = "\0";
    char backup_pass[50] = "\0";
    int posX = SystemContext::SSID_POS_X;
    int asciiChar = START_ASCII;
    uint64_t lastPressTime = time_us_64();
    strcpy(backup_ssid, ctx.SSID_WIFI);
    strcpy(backup_pass, ctx.PASS_WIFI);

    while(true) {
        if (xQueueReceive(ctx.gpio_action_q, &ctx.rot_btn_data.action_type, portMAX_DELAY)){
            switch (ctx.rot_btn_data.action_type){

                // ---------------- BACK PRESS ----------------
                case GpioAction::BACK_PRESS:
                    ctx.rot_btn_data.screen_type = ScreenType::SELECTION_SCR;
                    ctx.setpoint_temp = ctx.setpoint;
                    ctx.isEditing = false;
                    xQueueSend(ctx.gpio_data_q, &ctx.rot_btn_data, 0);
                    break;

                // ---------------- OK PRESS ----------------
                case GpioAction::OK_PRESS:
                    gpio_set_irq_enabled_with_callback(ctx.ok_btn, GPIO_IRQ_EDGE_FALL, false, &hw_ISR_CB);
                    if (time_us_64() - lastPressTime > 30000){ // debounce 30ms
                        lastPressTime = time_us_64();
                        if (ctx.rot_btn_data.screen_type == ScreenType::SELECTION_SCR){
                            if (ctx.selection_screen_option == 0){
                                ctx.rot_btn_data.screen_type = ScreenType::INFO_SCR;
                                xQueueSend(ctx.gpio_data_q, &ctx.rot_btn_data, 0);
                            } else if (ctx.selection_screen_option == 1){
                                ctx.rot_btn_data.screen_type = ScreenType::SET_CO2_SCR;
                                ctx.setpoint_temp = ctx.setpoint;
                                xQueueSend(ctx.gpio_data_q, &ctx.rot_btn_data, 0);
                            } else if (ctx.selection_screen_option == 2){
                                ctx.rot_btn_data.screen_type = ScreenType::WIFI_CONF_SCR;
                                xQueueSend(ctx.gpio_data_q, &ctx.rot_btn_data, 0);
                            }
                        } else if (ctx.rot_btn_data.screen_type == ScreenType::SET_CO2_SCR){
                            ctx.setpoint = ctx.setpoint_temp;
                            xEventGroupSetBits(ctx.co2EventGroup, SystemContext::CO2_CHANGE_BIT_EEPROM);
                        } else if (ctx.rot_btn_data.screen_type == ScreenType::WIFI_CONF_SCR){
                            if (!ctx.isEditing) {
                                // handle long/short press enter editing
                                int count = 0;
                                while (!gpio_get(ctx.ok_btn) && ++count < 150){
                                    vTaskDelay(pdMS_TO_TICKS(10));
                                }
                                if (count >= 150){ // long press to enter editing
                                    ctx.isEditing = true;
                                    switch (ctx.wifi_screen_option) {
                                        case 0:
                                            strcpy(ctx.SSID_WIFI, "");
                                            strcpy(backup_ssid, "");
                                            posX = SystemContext::SSID_POS_X;
                                            break;
                                        case 1:
                                            strcpy(ctx.PASS_WIFI, "");
                                            strcpy(backup_pass, "");
                                            posX = SystemContext::PASS_POS_X;
                                            break;
                                    }
                                    asciiChar = START_ASCII - 1;
                                    ctx.wifi_char_pos_X.pos_x = posX;
                                    ctx.wifi_char_pos_X.ch = asciiChar;
                                    xQueueSend(ctx.gpio_data_q, &ctx.rot_btn_data, 0);
                                    xQueueSend(ctx.wifiCharPos_q, &ctx.wifi_char_pos_X, 0);
                                } else { // short press -> save data
                                    printf("saving data...\n");
                                    xEventGroupSetBits(ctx.wifiEventGroup, SystemContext::WIFI_CHANGE_BIT_EEPROM);
                                }
                            } else {
                                // editing characters
                                int count = 0;
                                while (!gpio_get(ctx.ok_btn) && ++count < 150){
                                    vTaskDelay(pdMS_TO_TICKS(10));
                                }
                                if (count >= 150){ // long press exit editing
                                    ctx.isEditing = false;
                                    if (ctx.wifi_screen_option == 0){
                                        strcpy(ctx.SSID_WIFI, backup_ssid);
                                        ctx.SSID_WIFI[strlen(ctx.SSID_WIFI)] = '\0';
                                    } else {
                                        strcpy(ctx.PASS_WIFI, backup_pass);
                                        ctx.PASS_WIFI[strlen(ctx.PASS_WIFI)] = '\0';
                                    }
                                    xQueueSend(ctx.gpio_data_q, &ctx.rot_btn_data, 0);
                                } else { // short press confirm char
                                    char c[2];
                                    sprintf(c, "%c", asciiChar);
                                    if (ctx.wifi_screen_option == 0){
                                        strcat(ctx.SSID_WIFI, c);
                                        strcat(backup_ssid, c);
                                    } else {
                                        strcat(ctx.PASS_WIFI, c);
                                        strcat(backup_pass, c);
                                    }
                                    vTaskDelay(pdMS_TO_TICKS(5));
                                    posX += CHAR_WIDTH;
                                    if (posX > MAX_POS_X){
                                        posX -= CHAR_WIDTH;
                                        // scroll back 1 char
                                        if (ctx.wifi_screen_option == 0){
                                            for (int i = 0; i < strlen(ctx.SSID_WIFI); i++){
                                                ctx.SSID_WIFI[i] = ctx.SSID_WIFI[i+1];
                                            }
                                        } else {
                                            for (int i = 0; i < strlen(ctx.PASS_WIFI); i++){
                                                ctx.PASS_WIFI[i] = ctx.PASS_WIFI[i+1];
                                            }
                                        }
                                        ctx.wifi_char_pos_X.pos_x = posX;
                                        ctx.wifi_char_pos_X.ch = asciiChar;
                                        xQueueSend(ctx.gpio_data_q, &ctx.rot_btn_data, 0);
                                        xQueueSend(ctx.wifiCharPos_q, &ctx.wifi_char_pos_X, 0);
                                    }
                                    asciiChar = START_ASCII - 1;
                                    ctx.wifi_char_pos_X.pos_x = posX;
                                    ctx.wifi_char_pos_X.ch = asciiChar;
                                    xQueueSend(ctx.wifiCharPos_q, &ctx.wifi_char_pos_X, 0);
                                }
                            }
                        }
                    }
                    while (!gpio_get(ctx.ok_btn)) vTaskDelay(pdMS_TO_TICKS(10));
                    gpio_set_irq_enabled_with_callback(ctx.ok_btn, GPIO_IRQ_EDGE_FALL, true, &hw_ISR_CB);
                    break;

                // ---------------- ROTATE CW ----------------
                case GpioAction::ROT_CW:
                    if (ctx.rot_btn_data.screen_type == ScreenType::SELECTION_SCR){
                        ctx.selection_screen_option = (ctx.selection_screen_option + 1) % 3;
                        xQueueSend(ctx.gpio_data_q, &ctx.rot_btn_data, 0);
                    } else if (ctx.rot_btn_data.screen_type == ScreenType::SET_CO2_SCR){
                        ctx.setpoint_temp = (ctx.setpoint_temp < SystemContext::MAX_SETPOINT) ?
                                             ctx.setpoint_temp + 10 : SystemContext::MAX_SETPOINT;
                        xQueueSend(ctx.gpio_data_q, &ctx.rot_btn_data, 0);
                    } else if (ctx.rot_btn_data.screen_type == ScreenType::WIFI_CONF_SCR){
                        if (!ctx.isEditing){
                            ctx.wifi_screen_option = !ctx.wifi_screen_option;
                            xQueueSend(ctx.gpio_data_q, &ctx.rot_btn_data, 0);
                        } else {
                            if (++asciiChar > END_ASCII) asciiChar = START_ASCII;
                            ctx.wifi_char_pos_X.pos_x = posX;
                            ctx.wifi_char_pos_X.ch = asciiChar;
                            xQueueSend(ctx.wifiCharPos_q, &ctx.wifi_char_pos_X, 0);
                        }
                    }
                    break;

                // ---------------- ROTATE CCW ----------------
                case GpioAction::ROT_CCW:
                    if (ctx.rot_btn_data.screen_type == ScreenType::SELECTION_SCR){
                        ctx.selection_screen_option = (ctx.selection_screen_option + 2) % 3;
                        xQueueSend(ctx.gpio_data_q, &ctx.rot_btn_data, 0);
                    } else if (ctx.rot_btn_data.screen_type == ScreenType::SET_CO2_SCR){
                        ctx.setpoint_temp = (ctx.setpoint_temp > SystemContext::MIN_SETPOINT) ?
                                             ctx.setpoint_temp - 10 : SystemContext::MIN_SETPOINT;
                        xQueueSend(ctx.gpio_data_q, &ctx.rot_btn_data, 0);
                    } else if (ctx.rot_btn_data.screen_type == ScreenType::WIFI_CONF_SCR){
                        if (!ctx.isEditing){
                            ctx.wifi_screen_option = !ctx.wifi_screen_option;
                            xQueueSend(ctx.gpio_data_q, &ctx.rot_btn_data, 0);
                        } else {
                            if (--asciiChar < START_ASCII) asciiChar = END_ASCII;
                            ctx.wifi_char_pos_X.pos_x = posX;
                            ctx.wifi_char_pos_X.ch = asciiChar;
                            xQueueSend(ctx.wifiCharPos_q, &ctx.wifi_char_pos_X, 0);
                        }
                    }
                    break;

                default: break;
            }
        }
    }
}

// ---------------- MODBUS TASK -----------------
void modbus_task(void *param) {
    auto& ctx = SystemContext::getInstance();
    auto uart{std::make_shared<PicoOsUart>(SystemContext::UART_NR,
                                           SystemContext::UART_TX_PIN,
                                           SystemContext::UART_RX_PIN,
                                           SystemContext::BAUD_RATE,
                                           SystemContext::STOP_BITS)};
    auto rtu_client{std::make_shared<ModbusClient>(uart)};
    ModbusRegister mb_co2(rtu_client, 240, 256);
    ModbusRegister mb_rh(rtu_client, 241, 256);
    ModbusRegister mb_temp(rtu_client, 241, 257);
    ModbusRegister mb_fanSpeed(rtu_client, 1, 0);

    while (true) {
        ctx.co2 = mb_co2.read();
        vTaskDelay(pdMS_TO_TICKS(5));
        ctx.rh = mb_rh.read()/10;
        vTaskDelay(pdMS_TO_TICKS(5));
        ctx.temp = mb_temp.read()/10;
        vTaskDelay(pdMS_TO_TICKS(5));
        xEventGroupSetBits(ctx.co2EventGroup, SystemContext::CO2_CHANGE_BIT_LCD);
        vTaskDelay(pdMS_TO_TICKS(100));

        // Control logic remains the same, just using ctx.*
    }
}

// ---------------- BLINK TASK -----------------
void blink_task(void *param)
{
    auto lpr = (LedParams *) param;
    const uint led_pin = lpr->pin;
    const uint delay = pdMS_TO_TICKS(lpr->delay);
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);
    while (true) {
        gpio_put(led_pin, true);
        vTaskDelay(delay);
        gpio_put(led_pin, false);
        vTaskDelay(delay);
    }
}

// ---------------- MAIN -----------------
int main() {
    auto& ctx = SystemContext::getInstance();
    static LedParams lp1 = { .pin = 20, .delay = 5000 };
    stdio_init_all();
    printf("\nBoot\n");

    // Create queues and synchronization objects BEFORE tasks
    ctx.setpoint_q    = xQueueCreate(SystemContext::QUEUE_SIZE, sizeof(ctx.setpoint));
    ctx.wifiCharPos_q = xQueueCreate(SystemContext::QUEUE_SIZE, sizeof(WifiCharPos));
    ctx.gpio_data_q   = xQueueCreate(SystemContext::QUEUE_SIZE, sizeof(RotBtnData));
    ctx.gpio_action_q = xQueueCreate(SystemContext::QUEUE_SIZE, sizeof(GpioAction));
    ctx.gpio_sem      = xSemaphoreCreateBinary();
    ctx.wifiEventGroup = xEventGroupCreate();
    ctx.co2EventGroup  = xEventGroupCreate();

    if (!ctx.gpio_data_q || !ctx.gpio_action_q || !ctx.wifiCharPos_q) {
        printf("ERROR: Failed to create queues!\n");
        while(1);
    }

    // Create tasks
    xTaskCreate(gpio_task, "gpio task", 512, nullptr, tskIDLE_PRIORITY + 2, nullptr);
    xTaskCreate(display_task, "SSD1306", 1024, nullptr, tskIDLE_PRIORITY + 1, nullptr);
    xTaskCreate(modbus_task, "Modbus", 512, nullptr, tskIDLE_PRIORITY + 1, nullptr);
    // xTaskCreate(blink_task, "LED_1", 256, (void *) &lp1, tskIDLE_PRIORITY + 1, nullptr);

    printf("Starting FreeRTOS scheduler...\n");
    vTaskStartScheduler();

    // Should never reach here
    printf("ERROR: Scheduler failed to start!\n");
    while(true){};
}
