/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

 #include <stdio.h>
#include "pico/stdlib.h"

#include "btstack_run_loop.h"
#include "picow_bt_common.h"

#include "FreeRTOS.h"
#include "task.h"

#if HAVE_LWIP
#include "pico/lwip_freertos.h"
#include "pico/cyw43_arch.h"
#endif

void board_serve_task(__unused void *params);
void bt_main_task(__unused void *params);

int main() {
    stdio_init_all();
    printf("main starts\n");

    // If we're using lwip but not via cyw43 (e.g. pan) we have to call this
#if HAVE_LWIP && !CYW43_LWIP
    lwip_freertos_init(cyw43_arch_async_context());
#endif

    int res = picow_bt_init();
    if (res){
        return -1;
    }

    printf("after picow_bt_init\n");

    xTaskCreate(board_serve_task, "BoardServeThread", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1UL, NULL);
    xTaskCreate(bt_main_task, "BTMainThread", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2UL, NULL);

    /* Start the tasks and timer running. */
    vTaskStartScheduler();

    while(true) {
        vTaskDelay(1000);
    }

#if HAVE_LWIP && !CYW43_LWIP
    lwip_freertos_deinit(cyw43_arch_async_context());
#endif
}

void board_serve_task(__unused void *params) {
    printf("board_serve_task starts\n");
    while (true) {
        hal_led_toggle();
        vTaskDelay(200);
    }
}

void bt_main_task(__unused void *params){
    picow_bt_main();
    btstack_run_loop_execute();
}