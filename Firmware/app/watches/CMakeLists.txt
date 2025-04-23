cmake_minimum_required(VERSION 3.12)

set(COMMON_INC_DIR "${CMAKE_CURRENT_LIST_DIR}/../../inc")

# rest of your project
add_executable(watches
    src/main.c
)

target_compile_definitions(watches INTERFACE
    CYW43_TASK_STACK_SIZE=1024
    NO_SYS=0
    PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=1000
)

target_include_directories(watches PRIVATE ./inc 
    ${FREERTOS_KERNEL_PATH}/include/
    ${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/include/
    ${COMMON_INC_DIR}/config
    ${COMMON_INC_DIR}
)

target_link_libraries(watches PRIVATE pico_stdlib
    hardware_i2c
    pico_pcf8563
    FreeRTOS-Kernel-Heap4
)

# enable usb output, disable uart output
pico_enable_stdio_usb(watches 1)
pico_enable_stdio_uart(watches 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(watches)
