#pragma once

//---------------------------------
//-          LEDs
//---------------------------------
#define GPIO_LED_0   26
#define GPIO_LED_1   27
#define GPIO_LED_2   28
//---------------------------------

//---------------------------------
//-          I2C
//---------------------------------
#ifndef I2C_SSD1306_SDA
    #define I2C_SSD1306_SDA 2 
#endif //I2C_SSD1306_SDA

#ifndef I2C_SSD1306_SCL
    #define I2C_SSD1306_SCL 3 
#endif //I2C_SSD1306_SCL

#ifndef I2C_SSD1306_INST
    #define I2C_SSD1306_INST i2c1 
#endif //I2C_SSD1306_INST

#ifndef I2C_SSD1306_WIDTH
    #define I2C_SSD1306_WIDTH 128 
#endif //I2C_SSD1306_WIDTH

#ifndef I2C_SSD1306_HEIGHT
    #define I2C_SSD1306_HEIGHT 64
#endif //I2C_SSD1306_HEIGHT

#ifndef I2C_SSD1306_ADDR
    #define I2C_SSD1306_ADDR 0x3c
#endif //I2C_SSD1306_ADDR
//---------------------------------

//---------------------------------
//-          I2S
//---------------------------------
#define GPIO_I2S_DATA  4
#define GPIO_I2S_SCK   5
#define GPIO_I2S_SCL   6
//---------------------------------

//---------------------------------
//-        SD Card SPI
//---------------------------------
#define GPIO_SD_CARD_SPI_SCK   10
#define GPIO_SD_CARD_SPI_MOSI  11
#define GPIO_SD_CARD_SPI_MISO  8
#define GPIO_SD_CARD_SPI_CS    9
#define GPIO_SD_CARD_SPI_CARD_DETECT    12
#define GPIO_SD_CARD_SPI       spi1

// #define GPIO_SD_CARD_SPI_SCK   18
// #define GPIO_SD_CARD_SPI_MOSI  19
// #define GPIO_SD_CARD_SPI_MISO  16
// #define GPIO_SD_CARD_SPI_CS    17
// #define GPIO_SD_CARD_SPI_CARD_DETECT    29
// #define GPIO_SD_CARD_SPI       spi0

//---------------------------------

//---------------------------------
//-          Buttons
//---------------------------------
typedef enum {
    KEYCODE_NONE,
    KEYCODE_PLAY,
    KEYCODE_PAUSE,
    KEYCODE_STOP,
    KEYCODE_BACKWARD,
    KEYCODE_FORWARD,
    KEYCODE_REWIND,
    KEYCODE_FAST_FORWARD,
    KEYCODE_VOL_DOWN,
    KEYCODE_VOL_UP,
    KEYCODE_VOL_MUTE,
    KEYCODE_MODE,
    KEYCODE_PAIR
} KeyCode_e;

#define GPIO_BUTTON_COL_0   16
#define GPIO_BUTTON_COL_1   17
#define GPIO_BUTTON_COL_2   18
#define GPIO_BUTTON_COL_3   19
#define NUM_OF_GPIO_BUTTON_COLS  4
#define GPIO_BUTTON_COLS \
    {                        \
        GPIO_BUTTON_COL_0,   \
        GPIO_BUTTON_COL_1,   \
        GPIO_BUTTON_COL_2,   \
        GPIO_BUTTON_COL_3    \
    }

#define GPIO_BUTTON_ROW_0   20
#define GPIO_BUTTON_ROW_1   21
#define GPIO_BUTTON_ROW_2   22
#define NUM_OF_GPIO_BUTTON_ROWS  3
#define GPIO_BUTTON_ROWS \
    {                        \
        GPIO_BUTTON_ROW_0,   \
        GPIO_BUTTON_ROW_1,   \
        GPIO_BUTTON_ROW_2    \
    }

#define BUTTON_KEYCODES \
    {                                                                               \
        {KEYCODE_PLAY,     KEYCODE_PAUSE,    KEYCODE_STOP,         KEYCODE_MODE},   \
        {KEYCODE_BACKWARD, KEYCODE_REWIND,   KEYCODE_FAST_FORWARD, KEYCODE_FORWARD},\
        {KEYCODE_VOL_DOWN, KEYCODE_VOL_MUTE, KEYCODE_VOL_UP,       KEYCODE_PAIR}    \
    }

//---------------------------------