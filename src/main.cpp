#include <iostream>
#include <sstream>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "hardware/gpio.h"
#include "PicoOsUart.h"
#include "ssd1306.h"
#include "hardware/timer.h"
#include "cstring"

// Include SystemContext
#include "SystemContext.h"

extern "C" {
uint32_t read_runtime_ctr(void) {
    return timer_hw->timerawl;
}
}

#include "blinker.h"

// Forward declarations for tasks
void modbus_task(void *param);
void sensor_task(void *param);

int main()
{
    stdio_init_all();
    printf("\nBoot - Ventilation System\n");

    // Get the SystemContext singleton instance
    auto& ctx = SystemContext::getInstance();

    // Initialize all queues
    ctx.gpio_sem = xSemaphoreCreateBinary();
    ctx.setpoint_q = xQueueCreate(ctx.QUEUE_SIZE, sizeof(uint16_t));
    ctx.gpio_action_q = xQueueCreate(ctx.QUEUE_SIZE, sizeof(GpioAction));
    ctx.gpio_data_q = xQueueCreate(ctx.QUEUE_SIZE, sizeof(RotBtnData));
    ctx.wifiCharPos_q = xQueueCreate(ctx.QUEUE_SIZE, sizeof(WifiCharPos));

    // Initialize event groups
    ctx.wifiEventGroup = xEventGroupCreate();
    ctx.co2EventGroup = xEventGroupCreate();

    // Initialize default WiFi credentials (can be modified later)
    strcpy(ctx.SSID_WIFI, "");
    strcpy(ctx.PASS_WIFI, "");

    // Initialize sensor values
    ctx.co2 = 400;
    ctx.temp = 22;
    ctx.rh = 45;
    ctx.speed = 0;
    ctx.setpoint = 400;

    // Create display task (handles OLED display)
    xTaskCreate(SystemContext::displayTaskWrapper, "Display", 1024, nullptr,
                tskIDLE_PRIORITY + 2, nullptr);

    // Create GPIO task (handles buttons and rotary encoder)
    xTaskCreate(SystemContext::gpioTaskWrapper, "GPIO", 512, nullptr,
                tskIDLE_PRIORITY + 2, nullptr);

    // Create sensor reading task (Modbus communication with sensors)
    xTaskCreate(sensor_task, "Sensor", 512, nullptr,
                tskIDLE_PRIORITY + 1, nullptr);

    printf("Tasks created, starting scheduler...\n");

    // Start the FreeRTOS scheduler
    vTaskStartScheduler();

    // Should never reach here
    while(true){};
}

// ============ SENSOR TASK ============
// This task reads sensor data via Modbus and updates the display

#include "ModbusClient.h"
#include "ModbusRegister.h"

void sensor_task(void *param) {
    auto& ctx = SystemContext::getInstance();

    // Initialize LED and valve pins
    gpio_init(ctx.led1);
    gpio_set_dir(ctx.led1, GPIO_OUT);

    gpio_init(ctx.valve);
    gpio_set_dir(ctx.valve, GPIO_OUT);

    // Initialize Modbus communication
    auto uart{std::make_shared<PicoOsUart>(
        ctx.UART_NR, ctx.UART_TX_PIN, ctx.UART_RX_PIN,
        ctx.BAUD_RATE, ctx.STOP_BITS)};

    auto rtu_client{std::make_shared<ModbusClient>(uart)};

    // Create Modbus registers for sensor readings
    // Adjust slave ID and register addresses according to your sensor
    ModbusRegister rh_reg(rtu_client, 241, 256);   // Humidity register
    ModbusRegister temp_reg(rtu_client, 241, 257); // Temperature register
    ModbusRegister co2_reg(rtu_client, 241, 258);  // CO2 register (if available)

    printf("Sensor task started\n");

    while (true) {
        // Toggle LED to show task is running
        gpio_put(ctx.led1, !gpio_get(ctx.led1));

        // Read sensor values via Modbus
        ctx.rh = rh_reg.read() / 10;      // Convert to percentage
        vTaskDelay(pdMS_TO_TICKS(5));

        ctx.temp = temp_reg.read() / 10;  // Convert to degrees C
        vTaskDelay(pdMS_TO_TICKS(5));

        // If you have a CO2 sensor on Modbus, read it here
        // ctx.co2 = co2_reg.read();

        // For demo purposes, simulate CO2 if not available
        // Remove this if you have real CO2 sensor
        ctx.co2 = 400 + (ctx.temp * 10);  // Simulated value

        // Calculate fan speed based on CO2 level vs setpoint
        if (ctx.co2 > ctx.setpoint + ctx.OFFSET) {
            // CO2 too high, increase ventilation
            int diff = ctx.co2 - ctx.setpoint;
            ctx.speed = (diff * 100) / ctx.CO2_LIMIT;
            if (ctx.speed > 100) ctx.speed = 100;
            gpio_put(ctx.valve, 1);  // Open valve
        } else if (ctx.co2 < ctx.setpoint - ctx.OFFSET) {
            // CO2 low enough, reduce ventilation
            ctx.speed = 0;
            gpio_put(ctx.valve, 0);  // Close valve
        }
        // else: maintain current speed (hysteresis)

        // Notify display task to update if in info screen
        xEventGroupSetBits(ctx.co2EventGroup, ctx.CO2_CHANGE_BIT_LCD);

        printf("CO2:%d ppm, T:%d°C, RH:%d%%, Speed:%d%%, SP:%d ppm\n",
               ctx.co2, ctx.temp, ctx.rh, ctx.speed, ctx.setpoint);

        // Check if setpoint changed and needs to be saved to EEPROM
        EventBits_t bits = xEventGroupWaitBits(
            ctx.co2EventGroup,
            ctx.CO2_CHANGE_BIT_EEPROM,
            pdTRUE,  // Clear on exit
            pdFALSE, // Wait for any bit
            0);      // No wait

        if (bits & ctx.CO2_CHANGE_BIT_EEPROM) {
            printf("Saving new setpoint to EEPROM: %d ppm\n", ctx.setpoint);
            // Add EEPROM write code here if needed
        }

        // Check if WiFi credentials changed
        bits = xEventGroupWaitBits(
            ctx.wifiEventGroup,
            ctx.WIFI_CHANGE_BIT_EEPROM,
            pdTRUE,  // Clear on exit
            pdFALSE,
            0);

        if (bits & ctx.WIFI_CHANGE_BIT_EEPROM) {
            printf("Saving WiFi credentials to EEPROM\n");
            printf("SSID: %s\n", ctx.SSID_WIFI);
            printf("PASS: %s\n", ctx.PASS_WIFI);
            // Add EEPROM write code here if needed
            // You might also want to restart WiFi connection here
        }

        // Read sensor data every 3 seconds
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

// ============ ALTERNATIVE MAIN FOR TESTING WITHOUT MODBUS ============
// Uncomment this and comment out the sensor_task if you want to test
// the UI without actual sensor hardware

/*
void test_sensor_task(void *param) {
    auto& ctx = SystemContext::getInstance();

    gpio_init(ctx.led1);
    gpio_set_dir(ctx.led1, GPIO_OUT);

    printf("Test sensor task started (simulated data)\n");

    while (true) {
        gpio_put(ctx.led1, !gpio_get(ctx.led1));

        // Simulate changing sensor values
        static int counter = 0;
        ctx.co2 = 400 + (counter % 600);
        ctx.temp = 20 + (counter % 10);
        ctx.rh = 40 + (counter % 20);

        // Calculate fan speed
        if (ctx.co2 > ctx.setpoint + ctx.OFFSET) {
            int diff = ctx.co2 - ctx.setpoint;
            ctx.speed = (diff * 100) / ctx.CO2_LIMIT;
            if (ctx.speed > 100) ctx.speed = 100;
        } else if (ctx.co2 < ctx.setpoint - ctx.OFFSET) {
            ctx.speed = 0;
        }

        xEventGroupSetBits(ctx.co2EventGroup, ctx.CO2_CHANGE_BIT_LCD);

        counter++;
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
*/
