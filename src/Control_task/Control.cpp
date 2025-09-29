#include "Control.h"

Control::Control(SemaphoreHandle_t timer, uint32_t stack_size, UBaseType_t priority) :
    timer_semphr(timer) {

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

    // sensor objects
    CO2Sensor co2(rtu_client, 240, false);
    Fan fan(rtu_client, 1);

    while(true) {
        if (xSemaphoreTake(timer_semphr, portMAX_DELAY) == pdTRUE) {
            int co2_val = co2.readPpm();
            if (co2.isWorking()) {
                printf("CO2: %d ppm\n", co2_val);
            } else {
                printf("CO2 failed, last value: %d ppm\n", co2.lastPpm());
            }
            if (co2_val >= max_co2) {
                fan.setSpeed(50);
            } else {
                fan.setSpeed(0);
            }
        }
    }
}


