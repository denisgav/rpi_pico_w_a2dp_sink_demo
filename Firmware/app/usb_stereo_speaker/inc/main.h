#ifndef MAIN__H
#define MAIN__H

#include "board_defines.h"

#include "speaker_settings.h"

#define SAMPLE_BUFFER_SIZE  (96000/1000) // MAX sample rate divided by 1000. Size of 1 ms sample

#ifndef I2S_SPK_BPS
    #define I2S_SPK_BPS 32
#endif //I2S_SPK_BPS

#ifndef I2S_SPK_RATE_DEF
    #define I2S_SPK_RATE_DEF (48000)
#endif //I2S_SPK_RATE_DEF

#ifndef SPK_STATUS_LED
    #define SPK_STATUS_LED GPIO_LED_0
#endif //SPK_STATUS_LED

//-------------------------

#endif //MAIN__H