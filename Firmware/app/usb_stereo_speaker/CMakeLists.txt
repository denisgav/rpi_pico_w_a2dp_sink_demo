cmake_minimum_required(VERSION 3.12)

set(COMMON_INC_DIR "${CMAKE_CURRENT_LIST_DIR}/../../inc")

# rest of your project
add_executable(usb_stereo_speaker
    src/main.c
    src/usb_descriptors.c
    src/usb_speaker.c
)

target_include_directories(usb_stereo_speaker PRIVATE ./inc 
    ${COMMON_INC_DIR}/inc
)

target_link_libraries(usb_stereo_speaker PRIVATE 
    tinyusb_device tinyusb_board pico_stdlib pico_i2S_audio pico_volume_ctrl pico_ssd1306
)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(usb_stereo_speaker)
