cmake_minimum_required(VERSION 3.12)

set(COMMON_INC_DIR "${CMAKE_CURRENT_LIST_DIR}/../../inc")

set(BTSTACK_ROOT ${PICO_SDK_PATH}/lib/btstack)
set(BTSTACK_3RD_PARTY_PATH ${BTSTACK_ROOT}/3rd-party)

set(CONFIG_DIR "${COMMON_INC_DIR}/config")
set(INC_DIR "${CMAKE_CURRENT_LIST_DIR}/inc")
set(SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src")


##############################################################

add_executable(bt_a2dp_sink
    ${SRC_DIR}/picow_bt_common.c

    ${PICO_BTSTACK_PATH}/src/btstack_audio.c
    ${SRC_DIR}/btstack_audio_pico.c

    ${SRC_DIR}/a2dp_sink_demo.c
    ${SRC_DIR}/main.c
)

target_include_directories(bt_a2dp_sink PRIVATE ./inc
    ${FREERTOS_KERNEL_PATH}/include/
    ${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/include/
    ${CONFIG_DIR}/
    ${COMMON_INC_DIR}
)

target_compile_definitions(bt_a2dp_sink PRIVATE
    NO_SYS=0
    TEST_AUDIO=1
    CYW43_LWIP=0

    PICO_AUDIO_I2S_DATA_PIN=4
    PICO_AUDIO_I2S_CLOCK_PIN_BASE=5

    CYW43_TASK_STACK_SIZE=4096
    PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=1000

    #WANT_HCI_DUMP=1 # This enables btstack debug
    #ENABLE_SEGGER_RTT=1
)

target_link_libraries(bt_a2dp_sink PRIVATE
    pico_stdlib

    pico_btstack_ble 
    pico_btstack_classic
    pico_btstack_sbc_decoder 
    
    pico_btstack_cyw43
    pico_audio_i2s

    pico_keypad
    pico_ssd1306

    pico_cyw43_arch_sys_freertos
    FreeRTOS-Kernel-Heap4
)

if (EXISTS "${SRC_DIR}/${NAME}.gatt")
        pico_btstack_make_gatt_header(bt_a2dp_sink INTERFACE ${SRC_DIR}/a2dp_sink_demo.gatt)
endif()

pico_add_extra_outputs(bt_a2dp_sink)
#example_auto_set_url(bt_a2dp_sink)

# enable usb output, disable uart output
pico_enable_stdio_usb(bt_a2dp_sink 1)
pico_enable_stdio_uart(bt_a2dp_sink 0)


