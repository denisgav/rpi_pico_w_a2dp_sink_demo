#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <pico/stdlib.h>

//-----------------------------
#if HAVE_LWIP
#include "pico/lwip_freertos.h"
#include "pico/cyw43_arch.h"
#endif

#include "FreeRTOS.h"
#include "task.h"

#ifndef RUN_FREERTOS_ON_CORE
#define RUN_FREERTOS_ON_CORE 0
#endif

#define MAIN_TASK_PRIORITY           ( tskIDLE_PRIORITY + 2UL )
#define BLINK_TASK_PRIORITY          ( tskIDLE_PRIORITY + 0UL )
//-----------------------------

#include "bsp/board_api.h"
#include "tusb.h"

#include "main.h"
//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTOTYPES
//--------------------------------------------------------------------+

/* Blink pattern
 * - 250 ms  : device not mounted
 * - 1000 ms : device mounted
 * - 2500 ms : device is suspended
 */
enum {
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED = 1000,
  BLINK_SUSPENDED = 2500,
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

void main_task(__unused void *params);
void led_blinking_task(void* param);
void cdc_task(void *params);

void vLaunch( void);

//--------------------------------------------------------------------+
// Main
//--------------------------------------------------------------------+
int main(void){
  stdio_init_all();
  board_init();

  blink_interval_ms = BLINK_NOT_MOUNTED;

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

  // init device stack on configured roothub port
  // This should be called after scheduler/kernel is started.
  // Otherwise it could cause kernel issue since USB IRQ handler does use RTOS queue API.
  tusb_rhport_init_t dev_init = {
    .role = TUSB_ROLE_DEVICE,
    .speed = TUSB_SPEED_AUTO
  };
  tusb_init(BOARD_TUD_RHPORT, &dev_init);

  if (board_init_after_tusb) {
    board_init_after_tusb();
  }

  // Create task for: tinyusb, blinky, cdc
  TaskHandle_t led_blink_t;
  xTaskCreate(led_blinking_task, "LED_BlinkingTask", 4096, NULL, BLINK_TASK_PRIORITY, &led_blink_t);

  // RTOS forever loop
  while (1) {
    // put this thread to waiting state until there is new events
    tud_task();
  }

  #if HAVE_LWIP && !CYW43_LWIP
    lwip_freertos_deinit(cyw43_arch_async_context());
  #endif
}

void vLaunch( void) {
    TaskHandle_t task;
    xTaskCreate(main_task, "MainTask", 4096, NULL, MAIN_TASK_PRIORITY, &task);

#if NO_SYS && configUSE_CORE_AFFINITY && configNUMBER_OF_CORES > 1
    // we must bind the main task to one core (well at least while the init is called)
    // (note we only do this in NO_SYS mode, because cyw43_arch_freertos
    // takes care of it otherwise)
    vTaskCoreAffinitySet(task, 1);
#endif

    /* Start the tasks and timer running. */
    vTaskStartScheduler();
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) {
  blink_interval_ms = BLINK_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
  blink_interval_ms = BLINK_NOT_MOUNTED;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {
  (void) remote_wakeup_en;
  blink_interval_ms = BLINK_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
  blink_interval_ms = tud_mounted() ? BLINK_MOUNTED : BLINK_NOT_MOUNTED;
}


//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinking_task(void* param) {
  (void) param;
  static bool led_state = false;

  gpio_init(MSC_STATUS_LED);
  gpio_set_dir(MSC_STATUS_LED, GPIO_OUT);

  while(true){
    vTaskDelay(blink_interval_ms);

    board_led_write(led_state);
    gpio_put(MSC_STATUS_LED, led_state);
    led_state = 1 - led_state;
  }
}
