#pragma once

#define GPIO_BUTTON_PLAY_PAUSE_PIN   16
#define GPIO_BUTTON_BACKWARD_PIN     17
#define GPIO_BUTTON_REWIND_PIN       18
#define GPIO_BUTTON_FAST_FORWARD_PIN 19
#define GPIO_BUTTON_FORWARD_PIN      20

#define NUM_OF_GPIO_BUTTONS 5
#define GPIO_BUTTONS \
    {                                \
        GPIO_BUTTON_PLAY_PAUSE_PIN,  \
        GPIO_BUTTON_FORWARD_PIN,     \
        GPIO_BUTTON_BACKWARD_PIN,    \
        GPIO_BUTTON_REWIND_PIN,      \
        GPIO_BUTTON_FAST_FORWARD_PIN \
    }

