cmake_minimum_required(VERSION 3.12)

set(COMMON_INC_DIR "${CMAKE_CURRENT_LIST_DIR}/../../inc")

# rest of your project
add_executable(usb_msc
    src/main.c
    src/usb_descriptors.c
    src/msc_disk.c
)

target_compile_definitions(usb_msc INTERFACE
    CYW43_TASK_STACK_SIZE=4096
    NO_SYS=0
    PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=1000
)

target_include_directories(usb_msc PRIVATE ./inc 
    ${FREERTOS_KERNEL_PATH}/include/
    ${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/include/
    ${COMMON_INC_DIR}/config
    ${COMMON_INC_DIR}
)

target_link_libraries(usb_msc PRIVATE pico_stdlib
    FreeRTOS-Kernel-Heap4
    tinyusb_device tinyusb_board  
)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(usb_msc)
