cmake_minimum_required(VERSION 3.12)

set(COMMON_INC_DIR "${CMAKE_CURRENT_LIST_DIR}/../../inc")

# rest of your project
add_executable(blink
    src/main.c
)

target_compile_definitions(blink INTERFACE
    CYW43_TASK_STACK_SIZE=4096
    NO_SYS=0
    PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=1000
)

target_include_directories(blink PRIVATE ./inc 
    ${FREERTOS_KERNEL_PATH}/include/
    ${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/include/
    ${COMMON_INC_DIR}/config
    ${COMMON_INC_DIR}
)

target_link_libraries(blink PRIVATE pico_stdlib
    FreeRTOS-Kernel-Heap4
)

# enable usb output, disable uart output
pico_enable_stdio_usb(blink 1)
pico_enable_stdio_uart(blink 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(blink)
