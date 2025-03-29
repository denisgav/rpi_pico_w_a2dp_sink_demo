/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"

#include "picow_bt_common.h"

#if HAVE_LWIP
#include "pico/lwip_freertos.h"
#include "pico/cyw43_arch.h"
#endif

#include "FreeRTOS.h"
#include "task.h"

#include "board_defines.h"
#include "a2dp_sink_demo.h"

#ifndef RUN_FREERTOS_ON_CORE
#define RUN_FREERTOS_ON_CORE 0
#endif

#define TEST_TASK_PRIORITY				( tskIDLE_PRIORITY + 2UL )
#define BLINK_TASK_PRIORITY				( tskIDLE_PRIORITY + 1UL )
#define BUTTONS_HANDLE_TASK_PRIORITY	( tskIDLE_PRIORITY + 1UL )

void blink_task(__unused void *params);
void main_task(__unused void *params);
void vLaunch( void);

void buttons_handle_task(__unused void *params);

int main()
{
    stdio_init_all();

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

    int res = picow_bt_init();
    if (res){
        return;
    }

// If we're using lwip but not via cyw43 (e.g. pan) we have to call this
#if HAVE_LWIP && !CYW43_LWIP
    lwip_freertos_init(cyw43_arch_async_context());
#endif

    picow_bt_main();

    xTaskCreate(blink_task, "BlinkThread", configMINIMAL_STACK_SIZE, NULL, BLINK_TASK_PRIORITY, NULL);
    xTaskCreate(buttons_handle_task, "ButtonsHandleThread", configMINIMAL_STACK_SIZE, NULL, BUTTONS_HANDLE_TASK_PRIORITY, NULL);

    while(true) {
        vTaskDelay(1000);
    }

#if HAVE_LWIP && !CYW43_LWIP
    lwip_freertos_deinit(cyw43_arch_async_context());
#endif
}

void vLaunch( void) {
    TaskHandle_t task;
    xTaskCreate(main_task, "TestMainThread", 1024, NULL, TEST_TASK_PRIORITY, &task);

#if NO_SYS && configUSE_CORE_AFFINITY && configNUMBER_OF_CORES > 1
    // we must bind the main task to one core (well at least while the init is called)
    // (note we only do this in NO_SYS mode, because cyw43_arch_freertos
    // takes care of it otherwise)
    vTaskCoreAffinitySet(task, 1);
#endif

    /* Start the tasks and timer running. */
    vTaskStartScheduler();
}

void blink_task(__unused void *params) {
    printf("blink_task starts\n");
    while (true) {
        hal_led_toggle();
        vTaskDelay(200);
    }
}

void buttons_handle_task(__unused void *params){
    printf("buttons_handle_task starts\n");

    const int gpio_buttons[NUM_OF_GPIO_BUTTONS] = GPIO_BUTTONS;
    bool button_states[NUM_OF_GPIO_BUTTONS];


    for(int gpio_idx = 0; gpio_idx < NUM_OF_GPIO_BUTTONS; gpio_idx++){
        gpio_init(gpio_buttons[gpio_idx]);
        gpio_set_dir(gpio_buttons[gpio_idx], GPIO_IN);
        gpio_pull_down(gpio_buttons[gpio_idx]);
        // gpio_set_irq_enabled_with_callback(GPIO_BUTTON_PLAY_PAUSE_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &buttons_callback);

        button_states[gpio_idx] = false;
    }

    while (true) {
        // Read button states
        uint32_t gpio_all = gpio_get_all();
        //printf("gpio_all = 0x%x\n", gpio_all);
        for(int gpio_idx = 0; gpio_idx < NUM_OF_GPIO_BUTTONS; gpio_idx++){
            bool button_state = (gpio_all & (1 << gpio_buttons[gpio_idx])) != 0;

            //printf("gpio[%d] pin is %d, state is %d\n", gpio_idx, gpio_buttons[gpio_idx], button_state);

            if((button_state == true) && (button_states[gpio_idx] == false)){
                buttons_callback(gpio_buttons[gpio_idx], GPIO_IRQ_EDGE_RISE);
                //printf("gpio[%d] RISE\n", gpio_idx);
            }
            if((button_state == false) && (button_states[gpio_idx] == true)){
                buttons_callback(gpio_buttons[gpio_idx], GPIO_IRQ_EDGE_FALL);
                //printf("gpio[%d] FALL\n", gpio_idx);
            }

            button_states[gpio_idx] = button_state;
        }
        vTaskDelay(200);
    }

}
