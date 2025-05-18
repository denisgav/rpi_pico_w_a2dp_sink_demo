#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include "keypad.h"

static keypad_buttons_cb_t keypad_button_handler = NULL;

// Add a callback function
void set_keypad_button_handler(keypad_buttons_cb_t handler){
	keypad_button_handler = handler;
}

// Run button handle task
void keypad_buttons_handle_task(__unused void *params){
	printf("keypad_buttons_handle_task starts\n");

    const uint button_cols[NUM_OF_GPIO_BUTTON_COLS] = GPIO_BUTTON_COLS;
    const uint button_rows[NUM_OF_GPIO_BUTTON_ROWS] = GPIO_BUTTON_ROWS;

    bool button_states[NUM_OF_GPIO_BUTTON_ROWS][NUM_OF_GPIO_BUTTON_COLS];
    // Initialize button states
    for (int row_idx = 0; row_idx <  NUM_OF_GPIO_BUTTON_ROWS; row_idx++) {
        for (int col_idx = 0; col_idx < NUM_OF_GPIO_BUTTON_COLS; col_idx++) {
            button_states[row_idx][col_idx] = false;
        }
    }

    // Initialize GPIO
    for (int col_idx = 0; col_idx < NUM_OF_GPIO_BUTTON_COLS; col_idx++) {
        gpio_init(button_cols[col_idx]);
        gpio_set_dir(button_cols[col_idx], GPIO_OUT);
    
        gpio_put(button_cols[col_idx], 0);
    }
    
    for (int row_idx = 0; row_idx <  NUM_OF_GPIO_BUTTON_ROWS; row_idx++) {
        gpio_init(button_rows[row_idx]);
        gpio_set_dir(button_rows[row_idx], GPIO_IN);
    }

    // Scan bunnons
    while (true) {
        // Scan column by column
        if(keypad_button_handler != NULL){
	        for (int col_idx = 0; col_idx < NUM_OF_GPIO_BUTTON_COLS; col_idx++) {
	            gpio_put(button_cols[col_idx], 1);
	            vTaskDelay(5);

	            uint32_t gpio_all = gpio_get_all();
	            for (int row_idx = 0; row_idx < NUM_OF_GPIO_BUTTON_ROWS; row_idx++) {
	                bool button_state = (gpio_all & (1 << button_rows[row_idx])) != 0;

	                if((button_state == true) && (button_states[row_idx][col_idx] == false)){
	                    KeypadCode_e keyCode = GET_KEYPAD_BUTTON_CODE(col_idx, row_idx);
	                    keypad_button_handler(keyCode, KEYPAD_EDGE_RISE);
	                }
	                if((button_state == false) && (button_states[row_idx][col_idx] == true)){
	                    KeypadCode_e keyCode = GET_KEYPAD_BUTTON_CODE(col_idx, row_idx);
	                    keypad_button_handler(keyCode, KEYPAD_EDGE_FALL);
	                }
	    
	                button_states[row_idx][col_idx] = button_state;
	            }
	            gpio_put(button_cols[col_idx], 0);
	            vTaskDelay(5);
	        }
	    }
        vTaskDelay(50);
    }
}