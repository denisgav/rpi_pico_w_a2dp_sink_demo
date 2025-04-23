cmake_minimum_required(VERSION 3.12)

set(COMMON_INC_DIR "${CMAKE_CURRENT_LIST_DIR}/../../inc")

# rest of your project
add_executable(i2c_bus_scan
    src/main.c
)

target_compile_definitions(i2c_bus_scan INTERFACE
    PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=1000
)

target_include_directories(i2c_bus_scan PRIVATE ./inc 
    ${COMMON_INC_DIR}
)

target_link_libraries(i2c_bus_scan PRIVATE pico_stdlib
    hardware_i2c
)

# enable usb output, disable uart output
pico_enable_stdio_usb(i2c_bus_scan 1)
pico_enable_stdio_uart(i2c_bus_scan 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(i2c_bus_scan)
