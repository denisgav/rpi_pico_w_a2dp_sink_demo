#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <pico/stdlib.h>

#if HAVE_LWIP
#include "pico/lwip_freertos.h"
#include "pico/cyw43_arch.h"
#endif

#include "FreeRTOS.h"
#include "task.h"

#include "board_defines.h"

#include "keypad.h"

#include "ssd1306/ssd1306.h"


#ifndef RUN_FREERTOS_ON_CORE
#define RUN_FREERTOS_ON_CORE 0
#endif

#define MAIN_TASK_PRIORITY				  ( tskIDLE_PRIORITY + 2UL )
#define KEYPAD_TASK_PRIORITY              ( tskIDLE_PRIORITY + 1UL )

void keypad_buttons_handler(KeypadCode_e keyCode, KeypadEvent_e events);

void vLaunch( void);
void main_task(__unused void *params);

//---------------------------------------
//           SSD1306
//---------------------------------------
ssd1306_t disp;
void setup_ssd1306();
void display_ssd1306_info();
//---------------------------------------

bool key_status[NUM_OF_GPIO_BUTTON_ROWS][NUM_OF_GPIO_BUTTON_COLS];

int main(void){
	stdio_init_all();

    for(int row_idx = 0; row_idx < NUM_OF_GPIO_BUTTON_ROWS; row_idx++){
        for(int col_idx = 0; col_idx < NUM_OF_GPIO_BUTTON_COLS; col_idx++){
            key_status[row_idx][col_idx] = false;
        }
    }

    /* Configure the hardware ready to run the demo. */
    const char *rtos_name;
#if ( configNUMBER_OF_CORES > 1 )
    rtos_name = "FreeRTOS SMP";
#else
    rtos_name = "FreeRTOS";
#endif

#if ( configNUMBER_OF_CORES == 2 )
    printf("Starting %s on both cores:\n", rtos_name);
    vLaunch();
#elif ( RUN_FREE_RTOS_ON_CORE == 1 )
    printf("Starting %s on core 1:\n", rtos_name);
    multicore_launch_core1(vLaunch);
    while (true);
#else
    printf("Starting %s on core 0:\n", rtos_name);
    vLaunch();
#endif
    return 0;
}

void main_task(__unused void *params) {
    setup_ssd1306();

    set_keypad_button_handler(keypad_buttons_handler);

    TaskHandle_t task;
    xTaskCreate(keypad_buttons_handle_task, "KeypadTask", 1024, NULL, KEYPAD_TASK_PRIORITY, &task);

    while(true) {
        vTaskDelay(1000);
        printf("Hello World!\n");
    }

#if HAVE_LWIP && !CYW43_LWIP
    lwip_freertos_deinit(cyw43_arch_async_context());
#endif
}

void vLaunch( void) {
    TaskHandle_t task;
    xTaskCreate(main_task, "MainTask", 1024, NULL, MAIN_TASK_PRIORITY, &task);

#if NO_SYS && configUSE_CORE_AFFINITY && configNUMBER_OF_CORES > 1
    // we must bind the main task to one core (well at least while the init is called)
    // (note we only do this in NO_SYS mode, because cyw43_arch_freertos
    // takes care of it otherwise)
    vTaskCoreAffinitySet(task, 1);
#endif

    /* Start the tasks and timer running. */
    vTaskStartScheduler();
}

void keypad_buttons_handler(KeypadCode_e keyCode, KeypadEvent_e events){
    char fmt_tmp_str[20] = "";
    char cur_print_str[20] = "";
    printf("keypad_buttons_handler. keyCode = %d, events = %d\n", keyCode, events);

    uint8_t btn_col = GET_KEYPAD_BUTTON_COL(keyCode);
    uint8_t btn_row = GET_KEYPAD_BUTTON_ROW(keyCode);

    key_status[btn_row][btn_col] = (events == KEYPAD_EDGE_RISE) ? true : false;

    ssd1306_clear(&disp);

    ssd1306_draw_string(&disp, 4, 0, 1, "Keys status");

    memset(cur_print_str, 0x0, sizeof(cur_print_str));
    strcat(cur_print_str, " |");
    for(int col_idx = 0; col_idx < NUM_OF_GPIO_BUTTON_COLS; col_idx++){
        memset(fmt_tmp_str, 0x0, sizeof(fmt_tmp_str));
        itoa(col_idx, fmt_tmp_str, 10);
        strcat(cur_print_str, fmt_tmp_str);
        strcat(cur_print_str, "|");
    }
    ssd1306_draw_string(&disp, 0, 8, 1, cur_print_str);


    for(int row_idx = 0; row_idx < NUM_OF_GPIO_BUTTON_ROWS; row_idx++){
        memset(fmt_tmp_str, 0x0, sizeof(fmt_tmp_str));
        memset(cur_print_str, 0x0, sizeof(cur_print_str));

        itoa(row_idx, fmt_tmp_str, 10);
        strcat(cur_print_str, fmt_tmp_str);
        strcat(cur_print_str, "|");
        for(int col_idx = 0; col_idx < NUM_OF_GPIO_BUTTON_COLS; col_idx++){
            strcat(cur_print_str, key_status[row_idx][col_idx] ? "T" : "F");
            strcat(cur_print_str, "|");
        }
        ssd1306_draw_string(&disp, 0, 16+row_idx*8, 1, cur_print_str);
    }

    ssd1306_show(&disp);
}

//-------------------------
// SSD1306 functions
//-------------------------
void setup_ssd1306(){
  i2c_init(I2C_INST, 400000);
  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA);
  gpio_pull_up(I2C_SCL);

  disp.external_vcc=false;
  ssd1306_init(&disp, I2C_SSD1306_WIDTH, I2C_SSD1306_HEIGHT, I2C_SSD1306_ADDR, I2C_INST);
  ssd1306_clear(&disp);

  ssd1306_draw_string(&disp, 4, 16, 1, "Raspberry pi pico");
  ssd1306_draw_string(&disp, 8, 32, 1, "Keypad");
  ssd1306_show(&disp);
}
//-------------------------