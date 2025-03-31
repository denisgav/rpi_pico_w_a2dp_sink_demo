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
#define GPIO_I2C_SDA   2
#define GPIO_I2C_SCl   3
//---------------------------------

//---------------------------------
//-          I2S
//---------------------------------
#define GPIO_I2S_DATA  2
#define GPIO_I2S_SCK   3
#define GPIO_I2S_SCL   3
//---------------------------------

//---------------------------------
//-        SD Card SPI
//---------------------------------
#define GPIO_DS_CARD_SPI_SCK   2
#define GPIO_DS_CARD_SPI_MOSI  3
#define GPIO_DS_CARD_SPI_MISO  3
#define GPIO_DS_CARD_SPI_CS    3
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