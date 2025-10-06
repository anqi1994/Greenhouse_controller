// UI.h
#pragma once

#include <memory>
#include <cstdint>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"

#include "hardware/gpio.h"
#include "structs.h"   // Message, Monitored_data, MessageType
#include "GPIO/GPIO.h"      // your existing GPIO wrapper


// Encoder events (keep identical to your project-wide definition if you already have one)
enum encoderEv : int { ROT_L = 0, ROT_R = 1, SW = 2, OK_BTN = 3, BACK_BTN = 4 };

// Forward declarations (implemented in UI.cpp)
class PicoI2C;
class ssd1306os;

// UI task class: integrates encoder ISR and a 5-state UI state machine.
// States: WELCOME, MAIN_MENU, SHOW_INFO, CO2_SET, WIFI_CONFIG.
class UI {
public:
    // Queues:
    //   to_CO2     UI -> Control (send CO2_SET_DATA)
    //   to_Network UI -> Network (send CO2_SET_DATA)
    //   to_UI      Control/Network -> UI (receive MONITORED_DATA / CO2_SET_DATA)
    UI(QueueHandle_t to_CO2,
       QueueHandle_t to_Network,
       QueueHandle_t to_UI,
       uint32_t stack_size,
       UBaseType_t priority);

    UI(const UI&) = delete;
    UI& operator=(const UI&) = delete;

private:
    GPIO ok_;     // OK button on GPIO5 (input, pull-up, active-low)
    GPIO back_;   // BACK button on GPIO7 (input, pull-up, active-low)
    TickType_t last_ok_time_{0};
    TickType_t last_back_time_{0};


    // ===== FreeRTOS task entry wrappers =====
    static void task_wrap(void* pvParameters);
    void task_impl();  // main UI loop (5-state switch-case)

    // ===== Encoder ISR glue (Pico SDK has a single global callback) =====
    static void encoder_callback(uint gpio, uint32_t events);
    void handleEncoderCallback(uint gpio, uint32_t events);

    // ===== Helpers =====
    void send_setpoint(uint32_t value);  // send CO2_SET_DATA to Control + Network

private:
    // ===== Queues =====
    QueueHandle_t to_CO2_{nullptr};        // UI -> Control
    QueueHandle_t to_Network_{nullptr};    // UI -> Network
    QueueHandle_t to_UI_{nullptr};         // Control/Network -> UI
    QueueHandle_t encoder_queue_{nullptr}; // ROT_L / ROT_R / SW

    // ===== Encoder pins (using your GPIO wrapper) =====
    GPIO rotA_;   // encoder A (input, pull-up)
    GPIO rotB_;   // encoder B (input, pull-up)
    GPIO sw_;     // encoder push button (input, pull-up, active-low)

    // Debounce state (FreeRTOS ticks)
    TickType_t last_sw_time_{0};

    // ===== Display and I2C =====
    std::shared_ptr<PicoI2C> i2cbus1_;
    std::shared_ptr<ssd1306os> display_;

    // ===== Snapshot of last inbound monitored data =====
    Message rx_snapshot_{}; // holds last MONITORED_DATA

    // ===== Local UI state (no SystemContext) =====
    // CO2 setpoint and edit buffer
    int setpoint_{1200};
    int setpoint_edit_{1200};

    // Main menu cursor: 0=SHOW INFO, 1=CO2 SET, 2=WIFI CONFIG
    int menu_index_{0};

    // Wi-Fi fields (simple placeholders; editing逻辑可后续扩展)
    char ssid_[32]{ "MySSID" };
    char pass_[32]{ "MyPASS" };
    // Wi-Fi 子菜单光标：0=SSID, 1=Back
    int wifi_menu_index_{0};

    // ===== Singleton pointer for ISR trampoline =====
    static UI* instance_;

    // ===== Task name (for xTaskCreate) =====
    static constexpr const char* kTaskName = "ui_task";

    // ===== Setpoint constraints =====
    static constexpr int kMinSetpoint = 400;
    static constexpr int kMaxSetpoint = 2000;

    bool wifi_editing_{false};      // false: NAV, true: EDIT
    int  wifi_field_index_{0};      // 0=SSID, 1=PASS
    int  ascii_idx_{0};             // index in the selectable char set

    bool wifi_just_exited_edit_{false};

    // Cursor/geometry for ascii preview
    static constexpr int kCharW      = 8;    // glyph width
    static constexpr int kSSID_X0    = 44;   // text start X for SSID
    static constexpr int kPASS_X0    = 44;   // text start X for PASS
    static constexpr int kSSID_Y     = 20;   // baseline Y for SSID line
    static constexpr int kPASS_Y     = 35;   // baseline Y for PASS line
    static constexpr int kSSID_MAX   = 31;   // max visible chars
    static constexpr int kPASS_MAX   = 31;

    static constexpr const char* kCharSet =
        " ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789"
        "._-@";

};
