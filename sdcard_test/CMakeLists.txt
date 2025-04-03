cmake_minimum_required(VERSION 3.12)

set(PICO_BOARD pico)

add_executable(sdcard_test
        ./sdcard_test.c
        ./hw_config.c
)

target_link_libraries(sdcard_test PRIVATE 
        pico_stdlib
        FatFs_SPI
)

target_include_directories(sdcard_test PRIVATE 
        ../inc
        ./
)

# enable usb output, disable uart output
pico_enable_stdio_usb(sdcard_test 1)
pico_enable_stdio_uart(sdcard_test 0)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(sdcard_test)