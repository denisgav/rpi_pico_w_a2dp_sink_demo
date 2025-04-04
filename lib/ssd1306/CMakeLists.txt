add_library(pico_ssd1306 INTERFACE)

target_sources(pico_ssd1306 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/ssd1306.c
)

target_include_directories(pico_ssd1306 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/
)

target_link_libraries(pico_ssd1306 INTERFACE pico_stdlib hardware_i2c)