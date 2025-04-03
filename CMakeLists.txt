cmake_minimum_required(VERSION 3.12)

set(BTSTACK_ROOT ${PICO_SDK_PATH}/lib/btstack)
set(BTSTACK_3RD_PARTY_PATH ${BTSTACK_ROOT}/3rd-party)

set(CONFIG_DIR "${CMAKE_CURRENT_LIST_DIR}/config")
set(INC_DIR "${CMAKE_CURRENT_LIST_DIR}/inc")
set(SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src")

# Set the board to Raspberry Pi Pico W
set(PICO_BOARD pico_w)

# Include the subsidiary .cmake file to get the SDK
include(pico_sdk_import.cmake)

# We also need PICO EXTRAS
include(pico_extras_import.cmake)

#include FreeRTOS
include(FreeRTOS_Kernel_import.cmake)

# used for function 
# include(example_auto_set_url.cmake)

# Set the name and version of the project
project(rpi_pico_w_a2dp_sink_demo C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initialize the Pico SDK
pico_sdk_init()

add_library(pico_btstack_audio INTERFACE)
target_sources(pico_btstack_audio INTERFACE
        ${PICO_BTSTACK_PATH}/src/btstack_audio.c
        ${SRC_DIR}/btstack_audio_pico.c
        )
target_link_libraries(pico_btstack_audio INTERFACE
        pico_audio_i2s
        )

add_library(a2dp_sink_demo_pins INTERFACE)
target_compile_definitions(a2dp_sink_demo_pins INTERFACE
    PICO_AUDIO_I2S_DATA_PIN=4
    PICO_AUDIO_I2S_CLOCK_PIN_BASE=5
    )

# BT with disabled cyw43 lwip
add_library(picow_bt_no_cyw43_lwip INTERFACE)
target_link_libraries(picow_bt_no_cyw43_lwip INTERFACE
        picow_bt_common
        )
target_compile_definitions(picow_bt_no_cyw43_lwip INTERFACE
        CYW43_LWIP=0
        )

add_library(picow_bt_common_a2dp_sink_demo INTERFACE)
target_sources(picow_bt_common_a2dp_sink_demo INTERFACE
        # actual example
        ${SRC_DIR}/a2dp_sink_demo.c
        )
target_include_directories(picow_bt_common_a2dp_sink_demo INTERFACE
        ${INC_DIR}/
        ${SRC_DIR}/
        )

if (EXISTS "${SRC_DIR}/${NAME}.gatt")
        pico_btstack_make_gatt_header(picow_bt_common_a2dp_sink_demo INTERFACE ${SRC_DIR}/a2dp_sink_demo.gatt)
endif()

# Adds common stuff
add_library(picow_bt_common INTERFACE)
target_sources(picow_bt_common INTERFACE
        ${SRC_DIR}/picow_bt_common.c
        )
target_link_libraries(picow_bt_common INTERFACE
        pico_stdlib
        pico_btstack_cyw43
        pico_btstack_audio
        )
target_include_directories(picow_bt_common INTERFACE
        ${CONFIG_DIR}/
        )
target_compile_definitions(picow_bt_common INTERFACE
        )

        # variant: no cyw43 lwip | FreeRTOS
add_library(picow_bt_no_cyw43_lwip_freertos INTERFACE)
target_sources(picow_bt_no_cyw43_lwip_freertos INTERFACE
        ${SRC_DIR}/main.c
        )
target_include_directories(picow_bt_no_cyw43_lwip_freertos INTERFACE
        ${FREERTOS_KERNEL_PATH}/include/
        ${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/include/
        ${CONFIG_DIR}/
        )
target_compile_definitions(picow_bt_no_cyw43_lwip_freertos INTERFACE
        CYW43_TASK_STACK_SIZE=1024
        NO_SYS=0
        PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=3000
        TEST_AUDIO=1
        #WANT_HCI_DUMP=1 # This enables btstack debug
        #ENABLE_SEGGER_RTT=1
        )
target_link_libraries(picow_bt_no_cyw43_lwip_freertos INTERFACE
        picow_bt_no_cyw43_lwip
        pico_cyw43_arch_sys_freertos
        FreeRTOS-Kernel-Heap4
        )

add_executable(a2dp_sink_demo)
target_link_libraries(a2dp_sink_demo PRIVATE
picow_bt_common_a2dp_sink_demo
        picow_bt_no_cyw43_lwip_freertos
        pico_btstack_ble pico_btstack_classic # extra libs
        pico_btstack_sbc_decoder a2dp_sink_demo_pins
        )
pico_add_extra_outputs(a2dp_sink_demo)
#example_auto_set_url(a2dp_sink_demo)

# enable usb output, disable uart output
pico_enable_stdio_usb(a2dp_sink_demo 1)
pico_enable_stdio_uart(a2dp_sink_demo 0)

# Tell CMake where to find no-OS-FatFS-SD-SPI-RPi-Pico source code
add_subdirectory(lib/no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI build)
add_subdirectory("sdcard_test")