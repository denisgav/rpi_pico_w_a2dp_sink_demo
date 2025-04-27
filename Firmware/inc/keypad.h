#pragma once

#include "FreeRTOS.h"
#include "task.h"

#include "board_defines.h"

// Callback functions:
typedef void (*keypad_buttons_cb_t)(KeypadCode_e keyCode, KeypadEvent_e events);

// Add a callback function
void set_keypad_button_handler(keypad_buttons_cb_t handler);

// Run button handle task
void keypad_buttons_handle_task(__unused void *params);