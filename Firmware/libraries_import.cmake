cmake_minimum_required(VERSION 3.12)

###################################################
# Volume control library
###################################################
add_library(pico_volume_ctrl INTERFACE)

target_sources(pico_volume_ctrl INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src/volume_ctrl.c
)

target_include_directories(pico_volume_ctrl INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/inc
)

target_link_libraries(pico_volume_ctrl INTERFACE pico_stdlib)
###################################################

###################################################
# PDM microphone library
###################################################

add_library(pico_pdm_microphone INTERFACE)

target_sources(pico_pdm_microphone INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src/pdm/pdm_microphone.c
    ${CMAKE_CURRENT_LIST_DIR}/OpenPDM2PCM/OpenPDMFilter.c
    ${CMAKE_CURRENT_LIST_DIR}/src/ring_buf.c
)

target_include_directories(pico_pdm_microphone INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/inc
    ${CMAKE_CURRENT_LIST_DIR}/OpenPDM2PCM
)

pico_generate_pio_header(pico_pdm_microphone ${CMAKE_CURRENT_LIST_DIR}/src/pdm/pdm_microphone.pio)

target_link_libraries(pico_pdm_microphone INTERFACE pico_stdlib hardware_dma hardware_pio)
###################################################

###################################################
# I2S library
###################################################

add_library(pico_i2S_audio INTERFACE)

target_sources(pico_i2S_audio INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src/i2s/machine_i2s.c
    ${CMAKE_CURRENT_LIST_DIR}/src/ring_buf.c
)

target_include_directories(pico_i2S_audio INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/inc
)

pico_generate_pio_header(pico_i2S_audio ${CMAKE_CURRENT_LIST_DIR}/src/i2s/audio_i2s_tx_16b.pio)
pico_generate_pio_header(pico_i2S_audio ${CMAKE_CURRENT_LIST_DIR}/src/i2s/audio_i2s_tx_32b.pio)
pico_generate_pio_header(pico_i2S_audio ${CMAKE_CURRENT_LIST_DIR}/src/i2s/audio_i2s_rx_32b.pio)

target_link_libraries(pico_i2S_audio INTERFACE pico_stdlib hardware_dma hardware_pio)
###################################################

###################################################
# SSD1306 library
###################################################

add_library(pico_ssd1306 INTERFACE)

target_sources(pico_ssd1306 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src/ssd1306/ssd1306.c
)

target_include_directories(pico_ssd1306 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/inc
)

target_link_libraries(pico_ssd1306 INTERFACE pico_stdlib hardware_i2c)

###################################################

###################################################
# PCF8563 library
###################################################

add_library(pico_pcf8563 INTERFACE)

target_sources(pico_pcf8563 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src/pcf8563_i2c/pcf8563_i2c.c
)

target_include_directories(pico_pcf8563 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/inc
)

target_link_libraries(pico_pcf8563 INTERFACE pico_stdlib hardware_i2c)

###################################################

###################################################
# Keypad library
###################################################

add_library(pico_keypad INTERFACE)

target_compile_definitions(pico_keypad INTERFACE
    CYW43_TASK_STACK_SIZE=4096
    NO_SYS=0
    PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=1000
)

target_sources(pico_keypad INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src/keypad.c
)

target_include_directories(pico_keypad INTERFACE
    ${FREERTOS_KERNEL_PATH}/include/
    ${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/include/
    ${CMAKE_CURRENT_LIST_DIR}/inc/config
    ${CMAKE_CURRENT_LIST_DIR}/inc
)

target_link_libraries(pico_keypad INTERFACE pico_stdlib)

###################################################