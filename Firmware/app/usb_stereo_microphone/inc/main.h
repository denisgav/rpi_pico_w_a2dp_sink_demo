#ifndef MAIN__H
#define MAIN__H

#include "board_defines.h"

#ifndef I2S_MIC_BPS
    #define I2S_MIC_BPS 32 // 24 is not valid in this implementation, but INMP441 outputs 24 bits samples
#endif //I2S_MIC_BPS

#ifndef I2S_MIC_RATE_DEF
    #define I2S_MIC_RATE_DEF (48000)
#endif //I2S_MIC_RATE_DEF

#ifndef LED_RED_PIN
    #define LED_RED_PIN GPIO_LED_0
#endif //LED_RED_PIN

// #ifndef BTN_MUTE_PIN
//     #define BTN_MUTE_PIN 13
// #endif //BTN_MUTE_PIN

#include "microphone_settings.h"

// Comment this define to disable volume control
#define APPLY_VOLUME_FEATURE
#define FORMAT_24B_TO_24B_SHIFT_VAL 4u
#define FORMAT_24B_TO_16B_SHIFT_VAL 12u

typedef int32_t usb_audio_4b_sample;
typedef int16_t usb_audio_2b_sample;

#define USB_MIC_SAMPLE_BUFFER_SIZE  (I2S_MIC_RATE_DEF/1000) // MAX sample rate divided by 1000. Size of 1 ms sample


#endif //MAIN__H