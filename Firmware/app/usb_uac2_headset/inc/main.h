#ifndef MAIN__H
#define MAIN__H

#include "board_defines.h"

#include "usb_headset_settings.h"

#define SAMPLE_BUFFER_SIZE  (96000/1000) // MAX sample rate divided by 1000. Size of 1 ms sample

#ifndef I2S_SPK_BPS
    #define I2S_SPK_BPS 32
#endif //I2S_SPK_BPS

#ifndef I2S_SPK_RATE_DEF
    #define I2S_SPK_RATE_DEF (48000)
#endif //I2S_SPK_RATE_DEF

#ifndef I2S_MIC_BPS
    #define I2S_MIC_BPS 32 // 24 is not valid in this implementation, but INMP441 outputs 24 bits samples
#endif //I2S_MIC_BPS

#ifndef I2S_MIC_RATE_DEF
    #define I2S_MIC_RATE_DEF (48000)
#endif //I2S_MIC_RATE_DEF

#ifndef LED_RED_PIN
    #define LED_RED_PIN GPIO_LED_2
#endif //LED_RED_PIN

//-------------------------

#endif //MAIN__H