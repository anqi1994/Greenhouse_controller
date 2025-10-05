// UI.cpp
// UI task with 5-state switch-case FSM, integrated encoder ISR, and separate OK/BACK buttons.
// Queues: to_UI (inbound monitored/setpoint), to_CO2 (outbound setpoint), to_Network (outbound setpoint)

#include "UI.h"

#include <cstdio>
#include <cstring>
#include <string>
#include "PicoI2C.h"
#include "ssd1306os.h"
#include "icons.h"

// ---------------- Screen renderer (reuses your original drawing style) ----------------
class ScreenRenderer {
public:
    explicit ScreenRenderer(std::shared_ptr<ssd1306os> disp) : ssd_(std::move(disp)) {}

    // 1) Welcome: splash
    void welcome() {
        ssd_->fill(0);
        ssd_->text("GROUP 07", 28, 5);
        ssd_->text("Carol", 42, 22);
        ssd_->text("Julija", 38, 34);
        ssd_->text("Linh", 46, 46);
        ssd_->text("Qi", 54, 58);
        ssd_->show();
    }

    // 2) Main menu: highlight line by menu_index (0..2)
    void mainMenu(int menu_index) {
        int color0=1, color1=1, color2=1;
        ssd_->fill(0);
        switch (menu_index) {
            case 0: color0 = 0; ssd_->fill_rect(0, 0, 128, 11, 1); break;
            case 1: color1 = 0; ssd_->fill_rect(0,16, 128, 11, 1); break;
            case 2: color2 = 0; ssd_->fill_rect(0,30, 128, 11, 1); break;
            default: break;
        }
        ssd_->text("SHOW INFO",     28,  2, color0);
        ssd_->text("SET CO2 LEVEL",  8, 18, color1);
        ssd_->text("CONFIG WIFI",   20, 32, color2);
        ssd_->fill_rect(0, 53, 33, 10, 1);
        ssd_->text("OK", 1, 54, 0);
        ssd_->show();
    }

    // 3) Show info: co2/temp/rh/speed/setpoint
    void showInfo(int co2, int t, int h, int speed, int setpoint) {
        ssd_->fill(0);
        ssd_->text("CO2:" + std::to_string(co2) + " ppm", 38,  2);
        ssd_->text("RH:"  + std::to_string(h)   + " %",   38, 15);
        ssd_->text("T:"   + std::to_string(t)   + " C",   38, 28);
        ssd_->text("S:"   + std::to_string(speed)+ " %",  38, 41);
        ssd_->text("SP:"  + std::to_string(setpoint) + " ppm", 38, 54);
        ssd_->fill_rect(0, 0, 33, 10, 1);
        ssd_->text("BACK", 1, 1, 0);
        ssd_->show();
    }

    // 4) CO2 set screen: show editable value
    void co2Set(int val) {
        ssd_->fill(0);
        ssd_->text("CO2 LEVEL", 50, 20);
        const std::string txt = std::to_string(val) + " ppm";
        ssd_->text(txt, 55, 38);
        ssd_->fill_rect(0, 0, 33, 10, 1);
        ssd_->text("BACK", 1, 1, 0);
        ssd_->fill_rect(0, 53, 33, 10, 1);
        ssd_->text("OK", 1, 54, 0);
        ssd_->show();
    }

    // 5) Wi-Fi config: highlight field by wifi_index (0=SSID,1=PASS)
    void wifiConfig(int wifi_index, const char* ssid, const char* pass) {
        ssd_->fill(0);
        if (wifi_index == 0) ssd_->rect(0,17,128,13,1);
        else if (wifi_index == 1) ssd_->rect(0,32,128,13,1);

        ssd_->text("SSID:", 2, 20);
        ssd_->text(ssid,   44, 20);
        ssd_->text("PASS:", 2, 35);
        ssd_->text(pass,   44, 35);

        ssd_->fill_rect(0, 0, 33, 10, 1);
        ssd_->text("BACK", 1, 1, 0);
        ssd_->fill_rect(0, 53, 33, 10, 1);
        ssd_->text("OK", 1, 54, 0);
        ssd_->show();
    }

    // Optional: ascii preview (kept for compatibility)
    void asciiCharSelection(int x, int ch, int y) {
        char c[2]; c[0] = static_cast<char>(ch); c[1] = '\0';
        if (ch < 32) { ssd_->fill_rect(x, y, 8, 8, 1); }
        else { ssd_->fill_rect(x, y, 8, 8, 0); ssd_->text(c, x, y); }
        ssd_->show();
    }

private:
    std::shared_ptr<ssd1306os> ssd_;
};

// ===================== Static singleton for ISR =====================
UI* UI::instance_ = nullptr;

// ===================== Constructor =====================
UI::UI(QueueHandle_t to_CO2,
       QueueHandle_t to_Network,
       QueueHandle_t to_UI,
       uint32_t stack_size,
       UBaseType_t priority)
    : to_CO2_(to_CO2),
      to_Network_(to_Network),
      to_UI_(to_UI),
      rotA_(10, /*input=*/true, /*pullup=*/true, /*invert=*/false),
      rotB_(11, /*input=*/true, /*pullup=*/true, /*invert=*/false),
      sw_ (12, /*input=*/true, /*pullup=*/true, /*invert=*/false),
      ok_ (9,  /*input=*/true, /*pullup=*/true, /*invert=*/false),   // Separate OK button
      back_(7, /*input=*/true, /*pullup=*/true, /*invert=*/false)    // Separate BACK button
{
    // Initialize inbound snapshot
    rx_snapshot_.type = MONITORED_DATA;
    rx_snapshot_.data.co2_val = 0;
    rx_snapshot_.data.temperature = 0.0;
    rx_snapshot_.data.humidity = 0.0;
    rx_snapshot_.data.fan_speed = 0;
    rx_snapshot_.co2_set = 0;

    // Local defaults
    setpoint_ = 1200;
    setpoint_edit_ = setpoint_;
    menu_index_ = 0;
    wifi_menu_index_ = 0;
    std::snprintf(ssid_, sizeof(ssid_), "MySSID");
    std::snprintf(pass_, sizeof(pass_), "MyPASS");

    // Expose instance for ISR trampoline
    instance_ = this;

    // Encoder & buttons event queue
    encoder_queue_ = xQueueCreate(32, sizeof(encoderEv));
    configASSERT(encoder_queue_ != nullptr);
    vQueueAddToRegistry(encoder_queue_, "Encoder_Q");

    // Register IRQs: a single global callback via A channel registration
    gpio_set_irq_enabled_with_callback((uint)rotA_, GPIO_IRQ_EDGE_FALL, true, &UI::encoder_callback);
    // Enable falling-edge on other inputs; same global callback is already set
    gpio_set_irq_enabled((uint)sw_,   GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled((uint)ok_,   GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled((uint)back_, GPIO_IRQ_EDGE_FALL, true);

    // Create UI FreeRTOS task
    xTaskCreate(task_wrap, kTaskName, stack_size, this, priority, nullptr);
}

// ===================== ISR trampoline =====================
void UI::encoder_callback(uint gpio, uint32_t events) {
    if (instance_) instance_->handleEncoderCallback(gpio, events);
}

// ===================== ISR handler =====================
void UI::handleEncoderCallback(uint gpio, uint32_t events) {
    BaseType_t hpw = pdFALSE;
    encoderEv ev;
    TickType_t now_ticks = xTaskGetTickCountFromISR();

    // 1) Rotation: sample B on A falling edge to infer direction
    if ((gpio == (uint)rotA_) && (events & GPIO_IRQ_EDGE_FALL)) {
        // Swap ROT_L/ROT_R here if direction inverted in your wiring
        ev = (rotB_.read() == 0) ? ROT_L : ROT_R;
        (void)xQueueSendToBackFromISR(encoder_queue_, &ev, &hpw);
    }

    // 2) Encoder's own push button SW (optional confirm)
    if ((gpio == (uint)sw_) && (events & GPIO_IRQ_EDGE_FALL)) {
        if ((now_ticks - last_sw_time_) >= pdMS_TO_TICKS(250)) {
            ev = SW;
            (void)xQueueSendToBackFromISR(encoder_queue_, &ev, &hpw);
            last_sw_time_ = now_ticks;
        }
    }

    // 3) Separate OK button (GPIO5)
    if ((gpio == (uint)ok_) && (events & GPIO_IRQ_EDGE_FALL)) {
        if ((now_ticks - last_ok_time_) >= pdMS_TO_TICKS(120)) { // 120 ms debounce
            ev = OK_BTN;
            (void)xQueueSendToBackFromISR(encoder_queue_, &ev, &hpw);
            last_ok_time_ = now_ticks;
        }
    }

    // 4) Separate BACK button (GPIO7)
    if ((gpio == (uint)back_) && (events & GPIO_IRQ_EDGE_FALL)) {
        if ((now_ticks - last_back_time_) >= pdMS_TO_TICKS(120)) { // 120 ms debounce
            ev = BACK_BTN;
            (void)xQueueSendToBackFromISR(encoder_queue_, &ev, &hpw);
            last_back_time_ = now_ticks;
        }
    }

    portYIELD_FROM_ISR(hpw);
}

// ===================== Task entry wrapper =====================
void UI::task_wrap(void* pvParameters) {
    auto* self = static_cast<UI*>(pvParameters);
    self->task_impl();
}

// ===================== Helper: send setpoint to both queues =====================
void UI::send_setpoint(uint32_t value) {
    Message out{};
    out.type    = CO2_SET_DATA;
    out.co2_set = value;
    (void)xQueueSendToBack(to_CO2_,     &out, portMAX_DELAY);
    (void)xQueueSendToBack(to_Network_, &out, portMAX_DELAY);
}

// ===================== Main UI task (5-state FSM) =====================
void UI::task_impl() {
    // Init I2C and display
    i2cbus1_  = std::make_shared<PicoI2C>(1, 400000);
    display_  = std::make_shared<ssd1306os>(i2cbus1_);
    ScreenRenderer screen(display_);

    // Five explicit states
    enum class State : uint8_t { WELCOME = 0, MAIN_MENU, SHOW_INFO, CO2_SET, WIFI_CONFIG };
    State state = State::WELCOME;

    // Boot: show welcome screen for ~2 seconds or until user action
    screen.welcome();
    TickType_t welcome_deadline = xTaskGetTickCount() + pdMS_TO_TICKS(2000);

    for (;;) {
        // 1) Drain messages from to_UI_ (Control monitored data + Network setpoint)
        Message in{};
        while (xQueueReceive(to_UI_, &in, 0) == pdTRUE) {
            if (in.type == MONITORED_DATA) {
                rx_snapshot_.data.co2_val     = in.data.co2_val;
                rx_snapshot_.data.temperature = in.data.temperature;
                rx_snapshot_.data.humidity    = in.data.humidity;
                rx_snapshot_.data.fan_speed   = in.data.fan_speed;

                if (state == State::SHOW_INFO) {
                    screen.showInfo((int)rx_snapshot_.data.co2_val,
                                    (int)rx_snapshot_.data.temperature,
                                    (int)rx_snapshot_.data.humidity,
                                    (int)rx_snapshot_.data.fan_speed,
                                    setpoint_);
                }
            } else if (in.type == CO2_SET_DATA) {
                // Treat any CO2_SET_DATA received on to_UI_ as from Network
                setpoint_      = (int)in.co2_set;
                setpoint_edit_ = setpoint_;
                if (state == State::CO2_SET) {
                    screen.co2Set(setpoint_edit_);
                } else if (state == State::SHOW_INFO) {
                    screen.showInfo((int)rx_snapshot_.data.co2_val,
                                    (int)rx_snapshot_.data.temperature,
                                    (int)rx_snapshot_.data.humidity,
                                    (int)rx_snapshot_.data.fan_speed,
                                    setpoint_);
                }
            }
        }

        // 2) Encoder & button events
        encoderEv ev{};
        if (xQueueReceive(encoder_queue_, &ev, pdMS_TO_TICKS(10)) == pdTRUE) {
            switch (state) {
                case State::WELCOME: {
                    // Any action exits welcome to main menu
                    state = State::MAIN_MENU;
                    screen.mainMenu(menu_index_);
                } break;

                case State::MAIN_MENU: {
                    // 0: SHOW INFO, 1: CO2 SET, 2: WIFI CONFIG
                    if (ev == ROT_L) {
                        menu_index_ = (menu_index_ + 1) % 3;
                        screen.mainMenu(menu_index_);
                    } else if (ev == ROT_R) {
                        menu_index_ = (menu_index_ + 2) % 3; // -1 mod 3
                        screen.mainMenu(menu_index_);
                    } else if (ev == SW || ev == OK_BTN) {  // SW or OK enters submenu
                        if (menu_index_ == 0) {
                            state = State::SHOW_INFO;
                            screen.showInfo((int)rx_snapshot_.data.co2_val,
                                            (int)rx_snapshot_.data.temperature,
                                            (int)rx_snapshot_.data.humidity,
                                            (int)rx_snapshot_.data.fan_speed,
                                            setpoint_);
                        } else if (menu_index_ == 1) {
                            state = State::CO2_SET;
                            setpoint_edit_ = setpoint_;
                            screen.co2Set(setpoint_edit_);
                        } else {
                            state = State::WIFI_CONFIG;
                            wifi_menu_index_ = 0; // default highlight SSID
                            screen.wifiConfig(wifi_menu_index_, ssid_, pass_);
                        }
                    } else if (ev == BACK_BTN) {
                        // Option A: ignore; Option B: return to welcome
                        // state = State::WELCOME; screen.welcome();
                    }
                } break;

                case State::SHOW_INFO: {
                    if (ev == BACK_BTN || ev == SW) {
                        state = State::MAIN_MENU;
                        screen.mainMenu(menu_index_);
                    } else {
                        screen.showInfo((int)rx_snapshot_.data.co2_val,
                                        (int)rx_snapshot_.data.temperature,
                                        (int)rx_snapshot_.data.humidity,
                                        (int)rx_snapshot_.data.fan_speed,
                                        setpoint_);
                    }
                } break;

                case State::CO2_SET: {
                    if (ev == ROT_L) {
                        setpoint_edit_ = (setpoint_edit_ < kMaxSetpoint) ? (setpoint_edit_ + 10) : kMaxSetpoint;
                        screen.co2Set(setpoint_edit_);
                    } else if (ev == ROT_R) {
                        setpoint_edit_ = (setpoint_edit_ > kMinSetpoint) ? (setpoint_edit_ - 10) : kMinSetpoint;
                        screen.co2Set(setpoint_edit_);
                    } else if (ev == OK_BTN || ev == SW) {
                        // Confirm: update local and broadcast to Control + Network
                        setpoint_ = setpoint_edit_;
                        send_setpoint((uint32_t)setpoint_);
                        screen.co2Set(setpoint_edit_);
                    } else if (ev == BACK_BTN) {
                        state = State::MAIN_MENU;
                        screen.mainMenu(menu_index_);
                    }
                } break;

                case State::WIFI_CONFIG: {
                    // Simple two-item behavior demo: 0=SSID field, 1=PASS field
                    if (ev == ROT_L || ev == ROT_R) {
                        // Toggle between SSID and PASS
                        wifi_menu_index_ ^= 1;
                        screen.wifiConfig(wifi_menu_index_, ssid_, pass_);
                    } else if (ev == OK_BTN || ev == SW) {
                        // Placeholder: here you can enter an editing sub-state if needed
                        screen.wifiConfig(wifi_menu_index_, ssid_, pass_);
                    } else if (ev == BACK_BTN) {
                        state = State::MAIN_MENU;
                        screen.mainMenu(menu_index_);
                    }
                } break;
            } // switch(state)
        }     // if event

        // 3) Passive timeout from WELCOME to MAIN_MENU
        if (state == State::WELCOME && xTaskGetTickCount() >= welcome_deadline) {
            state = State::MAIN_MENU;
            screen.mainMenu(menu_index_);
        }
    } // for(;;)
}
