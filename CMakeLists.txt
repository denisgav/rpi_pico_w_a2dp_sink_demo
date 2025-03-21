cmake_minimum_required(VERSION 3.12)

set(BTSTACK_ROOT ${PICO_SDK_PATH}/lib/btstack)
set(BTSTACK_3RD_PARTY_PATH ${BTSTACK_ROOT}/3rd-party)

set(COMMON_DIR "${CMAKE_CURRENT_LIST_DIR}/common")
set(CONFIG_DIR "${CMAKE_CURRENT_LIST_DIR}/config")
set(INC_DIR "${CMAKE_CURRENT_LIST_DIR}/inc")
set(SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src")


# Set the board to Raspberry Pi Pico W
set(PICO_BOARD pico_w)

# Set top source code folder
set(SOURCE_FOLDER "src")

# Include the subsidiary .cmake file to get the SDK
include(pico_sdk_import.cmake)

# We also need PICO EXTRAS
include(pico_extras_import.cmake)

# used for function 
include(example_auto_set_url.cmake)

# Set the name and version of the project
project(rpi_pico_w_a2dp_sink_demo)

# initialize the Pico SDK
pico_sdk_init()

# Add an executable target for the project
#add_executable(${PROJECT_NAME})

if (NOT PICO_EXTRAS_PATH)
        message("Skipping some Pico W BTstack examples that require pico-extras")
else()
        add_library(pico_btstack_audio_example INTERFACE)
        target_sources(pico_btstack_audio_example INTERFACE
                ${PICO_BTSTACK_PATH}/src/btstack_audio.c
                ${COMMON_DIR}/btstack_audio_pico.c
                )
        target_link_libraries(pico_btstack_audio_example INTERFACE
                pico_audio_i2s
                )
endif()

# Adds common stuff for all the examples
add_library(picow_bt_example_common INTERFACE)
target_sources(picow_bt_example_common INTERFACE
        ${COMMON_DIR}/picow_bt_example_common.c
        )
target_link_libraries(picow_bt_example_common INTERFACE
        pico_stdlib
        pico_btstack_cyw43
        )
target_include_directories(picow_bt_example_common INTERFACE
        ${COMMON_DIR}/ # Use our own config
        ${CONFIG_DIR}/
        ${INC_DIR}/
        ${SRC_DIR}/
        )
target_compile_definitions(picow_bt_example_common INTERFACE
        PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=3000
        #WANT_HCI_DUMP=1 # This enables btstack debug
        #ENABLE_SEGGER_RTT=1
        )
if (TARGET pico_btstack_audio_example)
        target_link_libraries(picow_bt_example_common INTERFACE
                pico_btstack_audio_example
                )
        target_compile_definitions(picow_bt_example_common INTERFACE
                TEST_AUDIO=1
                )
endif()

# Adds stuff needed for cyw43 lwip
add_library(picow_bt_example_cyw43_lwip INTERFACE)
target_link_libraries(picow_bt_example_cyw43_lwip INTERFACE
        picow_bt_example_common
        )
target_compile_definitions(picow_bt_example_cyw43_lwip INTERFACE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        CYW43_LWIP=1
        )
target_link_libraries(picow_bt_example_cyw43_lwip INTERFACE
        pico_lwip_iperf
        )
target_include_directories(picow_bt_example_cyw43_lwip INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/../wifi # for our common lwipopts
        )

# disables cyw43 lwip
add_library(picow_bt_example_no_cyw43_lwip INTERFACE)
target_link_libraries(picow_bt_example_no_cyw43_lwip INTERFACE
        picow_bt_example_common
        )
target_compile_definitions(picow_bt_example_no_cyw43_lwip INTERFACE
        CYW43_LWIP=0
        )

# variant: no cyw43 lwip | background
add_library(picow_bt_example_no_cyw43_lwip_background INTERFACE)
target_sources(picow_bt_example_no_cyw43_lwip_background INTERFACE
        ${COMMON_DIR}/picow_bt_example_background.c
        )
target_link_libraries(picow_bt_example_no_cyw43_lwip_background INTERFACE
        picow_bt_example_no_cyw43_lwip
        pico_cyw43_arch_threadsafe_background
        )

# variant: cyw43 lwip | background
add_library(picow_bt_example_cyw43_lwip_background INTERFACE)
target_sources(picow_bt_example_cyw43_lwip_background INTERFACE
        ${COMMON_DIR}/picow_bt_example_background.c
        )
target_link_libraries(picow_bt_example_cyw43_lwip_background INTERFACE
        picow_bt_example_cyw43_lwip
        pico_cyw43_arch_lwip_threadsafe_background
        )

# variant: btstack lwip | background
add_library(picow_bt_example_btstack_lwip_background INTERFACE)
target_sources(picow_bt_example_btstack_lwip_background INTERFACE
        ${COMMON_DIR}/picow_bt_example_background.c
        )
target_link_libraries(picow_bt_example_btstack_lwip_background INTERFACE
        picow_bt_example_no_cyw43_lwip
        pico_cyw43_arch_threadsafe_background
        pico_lwip_nosys
        pico_btstack_bnep_lwip
        )
target_include_directories(picow_bt_example_btstack_lwip_background INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/../wifi # for our common lwipopts
        )

# Common stuff for all examples. Pass the example name
function(picow_bt_example_common NAME)
        if (NOT TARGET picow_bt_example_common_${NAME})
                add_library(picow_bt_example_common_${NAME} INTERFACE)
                target_sources(picow_bt_example_common_${NAME} INTERFACE
                        # actual example
                        ${SRC_DIR}/${NAME}.c
                        )
                if (EXISTS "${SRC_DIR}/${NAME}.gatt")
                        pico_btstack_make_gatt_header(picow_bt_example_common_${NAME} INTERFACE ${SRC_DIR}/${NAME}.gatt)
                endif()
        endif()
endfunction()

# Add an background example, pass btstack example name, target name, variant lib and extra libs
function(picow_bt_example_background NAME TARGET_NAME VARIANT_LIB)
        picow_bt_example_common(${NAME})
        add_executable(${TARGET_NAME}_background)
        target_link_libraries(${TARGET_NAME}_background PRIVATE
                picow_bt_example_common_${NAME}
                ${VARIANT_LIB}
                ${ARGN} # extra libs
                )
        pico_add_extra_outputs(${TARGET_NAME}_background)
        #example_auto_set_url(${TARGET_NAME}_background)
endfunction()


# The default name of the bt example target
function(picow_bt_example_target_name NAME RET)
        SET(${RET} "picow_bt_example_${NAME}" PARENT_SCOPE)
endfunction()

# Make a btstack example, NAME should match source file in lib/btstack/example
# Extra parameters indicate extra libraries to link to
function(picow_bt_example NAME)
        picow_bt_example_target_name(${NAME} TARGET_NAME)
        picow_bt_example_background(${NAME} ${TARGET_NAME} picow_bt_example_no_cyw43_lwip_background pico_btstack_ble pico_btstack_classic ${ARGN})
endfunction()



add_library(a2dp_sink_demo_pins INTERFACE)
target_compile_definitions(a2dp_sink_demo_pins INTERFACE
    PICO_AUDIO_I2S_DATA_PIN=9
    PICO_AUDIO_I2S_CLOCK_PIN_BASE=10
    )

picow_bt_example(a2dp_sink_demo pico_btstack_sbc_decoder a2dp_sink_demo_pins)
