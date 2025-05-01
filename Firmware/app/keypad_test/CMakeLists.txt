cmake_minimum_required(VERSION 3.12)

set(COMMON_INC_DIR "${CMAKE_CURRENT_LIST_DIR}/../../inc")

# rest of your project
add_executable(keypad_test
    src/main.c
)

target_compile_definitions(keypad_test INTERFACE
    CYW43_TASK_STACK_SIZE=1024
    NO_SYS=0
    PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=1000
)

target_include_directories(keypad_test PRIVATE ./inc 
    ${FREERTOS_KERNEL_PATH}/include/
    ${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/include/
    ${COMMON_INC_DIR}/config
    ${COMMON_INC_DIR}
)

target_link_libraries(keypad_test PRIVATE pico_stdlib
    pico_keypad
    pico_ssd1306
    FreeRTOS-Kernel-Heap4
)

# enable usb output, disable uart output
pico_enable_stdio_usb(keypad_test 1)
pico_enable_stdio_uart(keypad_test 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(keypad_test)
