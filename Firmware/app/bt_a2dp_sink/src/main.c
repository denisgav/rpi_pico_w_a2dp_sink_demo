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
#include "keypad.h"

#ifndef RUN_FREERTOS_ON_CORE
#define RUN_FREERTOS_ON_CORE 0
#endif

#define TEST_TASK_PRIORITY				( tskIDLE_PRIORITY + 2UL )
#define BLINK_TASK_PRIORITY				( tskIDLE_PRIORITY + 1UL )
#define KEYPAD_TASK_PRIORITY            ( tskIDLE_PRIORITY + 1UL )

void blink_task(__unused void *params);
void main_task(__unused void *params);
void vLaunch( void);

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
    
    set_keypad_button_handler(keypad_buttons_handler);

    TaskHandle_t task;
    xTaskCreate(keypad_buttons_handle_task, "KeypadTask", 1024, NULL, KEYPAD_TASK_PRIORITY, &task);

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
