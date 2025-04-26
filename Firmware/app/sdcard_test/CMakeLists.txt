cmake_minimum_required(VERSION 3.12)

set(COMMON_INC_DIR "${CMAKE_CURRENT_LIST_DIR}/../../inc")

add_executable(sdcard_test
        ./src/main.c
        ./src/hw_config.c
)

target_compile_definitions(sdcard_test INTERFACE
    PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=1000
)


target_link_libraries(sdcard_test PRIVATE pico_stdlib
        FatFs_SPI
)

target_include_directories(sdcard_test PRIVATE 
        ./inc
        ${COMMON_INC_DIR}
)

# enable usb output, disable uart output
pico_enable_stdio_usb(sdcard_test 1)
pico_enable_stdio_uart(sdcard_test 0)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(sdcard_test)