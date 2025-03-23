cmake_minimum_required(VERSION 3.12)

set(BTSTACK_ROOT ${PICO_SDK_PATH}/lib/btstack)
set(BTSTACK_3RD_PARTY_PATH ${BTSTACK_ROOT}/3rd-party)

set(COMMON_DIR "${CMAKE_CURRENT_LIST_DIR}/common")
set(CONFIG_DIR "${CMAKE_CURRENT_LIST_DIR}/config")
set(INC_DIR "${CMAKE_CURRENT_LIST_DIR}/inc")
set(SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src")


# Set the board to Raspberry Pi Pico W
set(PICO_BOARD pico_w)

# Include the subsidiary .cmake file to get the SDK
include(pico_sdk_import.cmake)

# We also need PICO EXTRAS
include(pico_extras_import.cmake)

# used for function 
# include(example_auto_set_url.cmake)

# Set the name and version of the project
project(rpi_pico_w_a2dp_sink_demo)

# initialize the Pico SDK
pico_sdk_init()

add_library(pico_btstack_audio INTERFACE)
target_sources(pico_btstack_audio INTERFACE
        ${PICO_BTSTACK_PATH}/src/btstack_audio.c
        ${COMMON_DIR}/btstack_audio_pico.c
        )
target_link_libraries(pico_btstack_audio INTERFACE
        pico_audio_i2s
        )

# Adds common stuff for all the examples
add_library(picow_bt_common INTERFACE)
target_sources(picow_bt_common INTERFACE
        ${COMMON_DIR}/picow_bt_common.c
        )
target_link_libraries(picow_bt_common INTERFACE
        pico_stdlib
        pico_btstack_cyw43
        pico_btstack_audio
        )
target_include_directories(picow_bt_common INTERFACE
        ${COMMON_DIR}/ # Use our own config
        ${CONFIG_DIR}/
        )
target_compile_definitions(picow_bt_common INTERFACE
        PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=3000
        TEST_AUDIO=1
        #WANT_HCI_DUMP=1 # This enables btstack debug
        #ENABLE_SEGGER_RTT=1
        )

# Adds stuff needed for cyw43 lwip
add_library(picow_bt_cyw43_lwip INTERFACE)
target_link_libraries(picow_bt_cyw43_lwip INTERFACE
        picow_bt_common
        )
target_compile_definitions(picow_bt_cyw43_lwip INTERFACE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        CYW43_LWIP=1
        )
target_link_libraries(picow_bt_cyw43_lwip INTERFACE
        pico_lwip_iperf
        )
target_include_directories(picow_bt_cyw43_lwip INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/../wifi # for our common lwipopts
        )

# disables cyw43 lwip
add_library(picow_bt_no_cyw43_lwip INTERFACE)
target_link_libraries(picow_bt_no_cyw43_lwip INTERFACE
        picow_bt_common
        )
target_compile_definitions(picow_bt_no_cyw43_lwip INTERFACE
        CYW43_LWIP=0
        )

# variant: no cyw43 lwip | background
add_library(picow_bt_no_cyw43_lwip_background INTERFACE)
target_sources(picow_bt_no_cyw43_lwip_background INTERFACE
        ${COMMON_DIR}/picow_bt_main_background.c
        )
target_link_libraries(picow_bt_no_cyw43_lwip_background INTERFACE
        picow_bt_no_cyw43_lwip
        pico_cyw43_arch_threadsafe_background
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



add_library(a2dp_sink_demo_pins INTERFACE)
target_compile_definitions(a2dp_sink_demo_pins INTERFACE
    PICO_AUDIO_I2S_DATA_PIN=9
    PICO_AUDIO_I2S_CLOCK_PIN_BASE=10
    )

add_executable(a2dp_sink_demo_background)
target_link_libraries(a2dp_sink_demo_background PRIVATE
picow_bt_common_a2dp_sink_demo
        picow_bt_no_cyw43_lwip_background
        pico_btstack_ble pico_btstack_classic # extra libs
        pico_btstack_sbc_decoder a2dp_sink_demo_pins
        )
pico_add_extra_outputs(a2dp_sink_demo_background)
#example_auto_set_url(a2dp_sink_demo_background)

# enable usb output, disable uart output
pico_enable_stdio_usb(a2dp_sink_demo_background 1)
pico_enable_stdio_uart(a2dp_sink_demo_background 0)