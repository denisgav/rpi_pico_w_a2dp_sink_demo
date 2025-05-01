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

#define FORMAT_24B_TO_24B_SHIFT_VAL 4u
#define FORMAT_24B_TO_16B_SHIFT_VAL 12u

typedef int32_t usb_audio_4b_sample;
typedef int16_t usb_audio_2b_sample;

#define USB_MIC_SAMPLE_BUFFER_SIZE  (I2S_MIC_RATE_DEF/1000) // MAX sample rate divided by 1000. Size of 1 ms sample

#ifndef SPK_STATUS_LED
    #define SPK_STATUS_LED GPIO_LED_0
#endif //SPK_STATUS_LED

#ifndef MIC_STATUS_LED
    #define MIC_STATUS_LED GPIO_LED_1
#endif //MIC_STATUS_LED

#ifndef MIC_MUTE_LED
    #define MIC_MUTE_LED GPIO_LED_2
#endif //MIC_MUTE_LED

//-------------------------

#endif //MAIN__H