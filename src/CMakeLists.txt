add_executable(${ProjectName}
        main.cpp
        critical_section.cpp
        Fmutex.cpp
        Fmutex.h
        blinker.cpp
        blinker.h

        ipstack/IPStack.cpp
        ipstack/IPStack.h
        ipstack/lwipopts.h
        ipstack/tls_common.c
        ipstack/picow_tls_client.c
        ipstack/lwipopts_tls.h

        i2c/PicoI2C.cpp
        i2c/PicoI2C.h

        uart/PicoOsUart.cpp
        uart/PicoOsUart.h

        modbus/nanomodbus.h
        modbus/nanomodbus.c
        modbus/ModbusRegister.cpp
        modbus/ModbusRegister.h
        modbus/ModbusClient.cpp
        modbus/ModbusClient.h

        display/framebuf.cpp
        display/framebuf.h
        display/mono_vlsb.cpp
        display/mono_vlsb.h
#        display/ssd1306.cpp
#        display/ssd1306.h
        display/ssd1306os.h
        display/ssd1306os.cpp
        ipstack/tls_common.h
        ipstack/tls_common.c
        display/screen_selection.h
        display/screen_selection.cpp
        Fan.cpp
        Fan.h

        Fan/Produal.cpp
        Fan/Produal.h
        CO2_sensor/GMP252.cpp
        CO2_sensor/GMP252.h
        T_RH_sensor/HMP60.cpp
        T_RH_sensor/HMP60.cpp
        T_RH_sensor/HMP60.h
        Structs.h
        Structs.h
        GPIO/GPIO.cpp
        Valve/Valve.cpp
        Valve/Valve.h
        Task_Network/Network.cpp
        Task_Network/Network.h
        Task_UI/UI.cpp
        Task_UI/UI.h
        Task_Control/Control.cpp
        Task_Control/Control.h
        EEPROM/EEPROM.cpp
        EEPROM/EEPROM.h
        Pressure_sensor/SDP610.cpp
        Pressure_sensor/SDP610.h
)

target_include_directories(${ProjectName} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
        modbus
        uart
        i2c
        display
        ipstack
)

target_compile_definitions(${ProjectName} PRIVATE
        WIFI_SSID=\"$ENV{WIFI_SSID}\"
        WIFI_PASSWORD=\"$ENV{WIFI_PASSWORD}\"
        NO_SYS=0            # don't want NO_SYS (generally this would be in your lwipopts.h)
        PICO_CYW43_ARCH_DEFAULT_COUNTRY_CODE=CYW43_COUNTRY_FINLAND
)

target_link_libraries(${ProjectName} 
        pico_stdlib
        hardware_i2c
        FreeRTOS-Kernel-Heap4
        pico_cyw43_arch_lwip_sys_freertos
        pico_lwip_mbedtls
        pico_mbedtls

)

# Ignore warnings from lwip code
set_source_files_properties(
        ${PICO_LWIP_PATH}/src/apps/altcp_tls/altcp_tls_mbedtls.c
        PROPERTIES
        COMPILE_OPTIONS "-Wno-unused-result"
)

pico_add_extra_outputs(${ProjectName})

# Disable usb output, enable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 1)
