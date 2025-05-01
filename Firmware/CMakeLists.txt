cmake_minimum_required(VERSION 3.12)

# initialize pico_sdk from GIT
# (note this can come from environment, CMake cache etc)
# set(PICO_SDK_FETCH_FROM_GIT on)

# pico_sdk_import.cmake is a single file copied from this SDK
# note: this must happen before project()

# Include the subsidiary .cmake file to get the SDK
include(pico_sdk_import.cmake)

#include FreeRTOS
include(FreeRTOS_Kernel_import.cmake)

# Set the name and version of the project
project(rpi_pico_w_a2dp_sink_demo C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initialize the Pico SDK
pico_sdk_init()

# include libraries
include(libraries_import.cmake)

# Tell CMake where to find no-OS-FatFS-SD-SPI-RPi-Pico source code
add_subdirectory(lib/no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI build)

add_subdirectory("app/hello_world")
add_subdirectory("app/blink")
add_subdirectory("app/i2c_bus_scan")
add_subdirectory("app/watches")
add_subdirectory("app/sdcard_test")
add_subdirectory("app/keypad_test")

add_subdirectory("app/usb_hid_multimedia_keypad")
add_subdirectory("app/usb_stereo_microphone")
add_subdirectory("app/usb_stereo_speaker")
add_subdirectory("app/usb_uac2_headset")