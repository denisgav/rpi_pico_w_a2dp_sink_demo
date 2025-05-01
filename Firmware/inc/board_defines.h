#pragma once

//---------------------------------
//-          LEDs
//---------------------------------
#define GPIO_LED_0   26
#define GPIO_LED_1   27
#define GPIO_LED_2   28
#define GPIO_LEDS \
    {                 \
        GPIO_LED_0,   \
        GPIO_LED_1,   \
        GPIO_LED_2    \
    }
//---------------------------------

//---------------------------------
//-          I2C
//---------------------------------
#ifndef I2C_SDA
    #define I2C_SDA 2 
#endif //I2C_SDA

#ifndef I2C_SCL
    #define I2C_SCL 3 
#endif //I2C_SCL

#ifndef I2C_INST
    #define I2C_INST i2c1 
#endif //I2C_INST
//---------------------------------

//---------------------------------
//-          I2C SSD1306
//---------------------------------
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
//-          I2C PCF8563 RTC
//---------------------------------
#define GPIO_I2C_PCF8563_INT   7
//---------------------------------

//---------------------------------
//-          I2S Speaker
//---------------------------------
#define GPIO_I2S_SPK_DATA  4
#define GPIO_I2S_SPK_SCK   5
#define GPIO_I2S_SPK_WS   (GPIO_I2S_SPK_SCK + 1)
//---------------------------------

//---------------------------------
//-          I2S Microphone
//---------------------------------
#define GPIO_I2S_MIC_DATA  13
#define GPIO_I2S_MIC_SCK   14
#define GPIO_I2S_MIC_WS   (GPIO_I2S_MIC_SCK + 1)
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

//---------------------------------

//---------------------------------
//-          Buttons
//---------------------------------

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

#define NUM_OF_KEYPAD_BUTTONS (NUM_OF_GPIO_BUTTON_COLS * NUM_OF_GPIO_BUTTON_ROWS)

typedef enum {
    KEYPAD_BTN_0_0 = 0,
    KEYPAD_BTN_0_1,
    KEYPAD_BTN_0_2,
    KEYPAD_BTN_0_3,

    KEYPAD_BTN_1_0,
    KEYPAD_BTN_1_1,
    KEYPAD_BTN_1_2,
    KEYPAD_BTN_1_3,

    KEYPAD_BTN_2_0,
    KEYPAD_BTN_2_1,
    KEYPAD_BTN_2_2,
    KEYPAD_BTN_2_3,
} KeypadCode_e;

typedef enum {
    KEYPAD_EDGE_RISE = 1,
    KEYPAD_EDGE_FALL = 2
} KeypadEvent_e;

#define GET_KEYPAD_BUTTON_CODE(col_idx, row_idx) ((KeypadCode_e)(row_idx*NUM_OF_GPIO_BUTTON_COLS + col_idx))
#define GET_KEYPAD_BUTTON_ROW(keyCode)  (keyCode / NUM_OF_GPIO_BUTTON_COLS)
#define GET_KEYPAD_BUTTON_COL(keyCode)  (keyCode % NUM_OF_GPIO_BUTTON_COLS)

//---------------------------------