/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <pico/stdlib.h>

#ifndef HID_STATUS_LED
  #define HID_STATUS_LED GPIO_LED_0
#endif //HID_STATUS_LED

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
#define STATUS_UPDATE_TASK_PRIORITY  ( tskIDLE_PRIORITY + 1UL )
//-----------------------------

#include "bsp/board_api.h"
#include "tusb.h"

#include "usb_descriptors.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+

/* Blink pattern
 * - 250 ms  : device not mounted
 * - 1000 ms : device mounted
 * - 2500 ms : device is suspended
 */
enum  {
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED = 1000,
  BLINK_SUSPENDED = 2500,
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

#include "ssd1306/ssd1306.h"

#include "keypad.h"

#define MAIN_TASK_PRIORITY           ( tskIDLE_PRIORITY + 2UL )
#define BLINK_TASK_PRIORITY          ( tskIDLE_PRIORITY + 0UL )
#define STATUS_UPDATE_TASK_PRIORITY  ( tskIDLE_PRIORITY + 1UL )
#define KEYPAD_TASK_PRIORITY         ( tskIDLE_PRIORITY + 1UL )

void led_blinking_task(__unused void *params);
void status_update_task(__unused void *params);
void main_task(__unused void *params);

void keypad_buttons_handler(KeypadCode_e keyCode, KeypadEvent_e events);
void on_button_press(uint8_t keycode, KeypadEvent_e events);

void vLaunch( void);

bool status_updated = false;

//---------------------------------------
//           SSD1306
//---------------------------------------
ssd1306_t disp;
void setup_ssd1306();
void display_ssd1306_info();
//---------------------------------------

int main(void){
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
  board_init();

  // init device stack on configured roothub port
  tusb_rhport_init_t dev_init = {
    .role = TUSB_ROLE_DEVICE,
    .speed = TUSB_SPEED_AUTO
  };
  tusb_init(BOARD_TUD_RHPORT, &dev_init);

  if (board_init_after_tusb) {
    board_init_after_tusb();
  }

  setup_ssd1306();

  TaskHandle_t led_blink_t;
  xTaskCreate(led_blinking_task, "LED_BlinkingTask", 4096, NULL, BLINK_TASK_PRIORITY, &led_blink_t);

  TaskHandle_t status_update_t;
  xTaskCreate(status_update_task, "StatusUpdateTask", 4096, NULL, STATUS_UPDATE_TASK_PRIORITY, &status_update_t);

  set_keypad_button_handler(keypad_buttons_handler);

  TaskHandle_t keypad_task;
  xTaskCreate(keypad_buttons_handle_task, "KeypadTask", 1024, NULL, KEYPAD_TASK_PRIORITY, &keypad_task);

  while (1)
  {
    tud_task(); // tinyusb device task
    vTaskDelay(0);
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

  ssd1306_draw_string(&disp, 4, 0, 1, "Raspberry pi pico");
  ssd1306_draw_string(&disp, 4, 16, 1, "USB HID media keypad");
  ssd1306_show(&disp);
}

//-------------------------

//-------------------------
// keypad callback functions
//-------------------------
void keypad_buttons_handler(KeypadCode_e keyCode, KeypadEvent_e events){
  switch(keyCode){
    case KEYPAD_BTN_0_0:{
        on_button_press(TUD_HID_CONSUMER_MUTE_CODE, events);
      break;
    }
    case KEYPAD_BTN_0_1:{
        on_button_press(TUD_HID_CONSUMER_VOLUME_DECREMENT_CODE, events);
      break;
    }
    case KEYPAD_BTN_0_2:{
        on_button_press(TUD_HID_CONSUMER_VOLUME_INCREMENT_CODE, events);
      break;
    }
    default:{
      break;
    }
  }
}

void on_button_press(uint8_t keycode, KeypadEvent_e events){
  // Remote wakeup
  if ( tud_suspended() && events == KEYPAD_EDGE_RISE)
  {
    // Wake up host if we are in suspend mode
    // and REMOTE_WAKEUP feature is enabled by host
    tud_remote_wakeup();
  }

  /*------------- Keyboard -------------*/
  if ( tud_hid_n_ready(ITF_NUM_HID1) )
  {
    // use to avoid send multiple consecutive zero report for keyboard
    static bool has_key = false;

    if ( events == KEYPAD_EDGE_RISE ){
      has_key = true;
      tud_hid_report(REPORT_ID_CONSUMER_CONTROL,
          &(keycode), sizeof(keycode));

    } else {
      // send empty key report if previously has key pressed
      if (has_key) {
        uint8_t empty_scan_code = 0x0;
        tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &(empty_scan_code),
            sizeof(empty_scan_code));
      }
      has_key = false;
    }
  }
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
  status_updated = true;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  blink_interval_ms = BLINK_NOT_MOUNTED;
  status_updated = true;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
  blink_interval_ms = BLINK_SUSPENDED;
  status_updated = true;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
  blink_interval_ms = tud_mounted() ? BLINK_MOUNTED : BLINK_NOT_MOUNTED;
  status_updated = true;
}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

void hid_task(void)
{
  // // Poll every 10ms
  // const uint32_t interval_ms = 10;
  // static uint32_t start_ms = 0;

  // if ( board_millis() - start_ms < interval_ms) return; // not enough time
  // start_ms += interval_ms;

  // uint32_t const btn = board_button_read();

  // // Remote wakeup
  // if ( tud_suspended() && btn )
  // {
  //   // Wake up host if we are in suspend mode
  //   // and REMOTE_WAKEUP feature is enabled by host
  //   tud_remote_wakeup();
  // }

  // /*------------- Keyboard -------------*/
  // if ( tud_hid_n_ready(ITF_KEYBOARD) )
  // {
  //   // use to avoid send multiple consecutive zero report for keyboard
  //   static bool has_key = false;

  //   if ( btn )
  //   {
  //     uint8_t keycode[6] = { 0 };
  //     keycode[0] = HID_KEY_A;

  //     tud_hid_n_keyboard_report(ITF_KEYBOARD, 0, 0, keycode);

  //     has_key = true;
  //   }else
  //   {
  //     // send empty key report if previously has key pressed
  //     if (has_key) tud_hid_n_keyboard_report(ITF_KEYBOARD, 0, 0, NULL);
  //     has_key = false;
  //   }
  // }

  // /*------------- Mouse -------------*/
  // if ( tud_hid_n_ready(ITF_MOUSE) )
  // {
  //   if ( btn )
  //   {
  //     int8_t const delta = 5;

  //     // no button, right + down, no scroll pan
  //     tud_hid_n_mouse_report(ITF_MOUSE, 0, 0x00, delta, delta, 0, 0);
  //   }
  // }
}


// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
  // TODO not Implemented
  (void) itf;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) reqlen;

  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
  // TODO set LED based on CAPLOCK, NUMLOCK etc...
  (void) itf;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) bufsize;
}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinking_task(__unused void *params){
  gpio_init(GPIO_LED_0);
  gpio_set_dir(GPIO_LED_0, GPIO_OUT);

  bool led_state = false;
  while(true){
    vTaskDelay(blink_interval_ms);

    //board_led_write(led_state);
    gpio_put(GPIO_LED_0, led_state);
    led_state = 1 - led_state;
  }
}

//--------------------------------------------------------------------+
// STATUS UPDATE TASK
//--------------------------------------------------------------------+
void status_update_task(__unused void *params){
  while(true){
    vTaskDelay(500);

    if(status_updated == true){
      status_updated = false;
      display_ssd1306_info();
    }
  }

}

void display_ssd1306_info(void) {
  char fmt_tmp_str[20] = "";

  ssd1306_clear(&disp);

  switch (blink_interval_ms) {
    case BLINK_NOT_MOUNTED: {
      ssd1306_draw_string(&disp, 4, 0, 1, "HID not mounted");
      break;
    }
    case BLINK_SUSPENDED: {
      ssd1306_draw_string(&disp, 4, 0, 1, "HID suspended");
      break;
    }
    case BLINK_MOUNTED: {
      ssd1306_draw_string(&disp, 4, 0, 1, "HID mounted");
      break;
    }
    default: {
      ssd1306_draw_string(&disp, 4, 0, 1, "HID unknown");
      break;
    }
  }
  ssd1306_show(&disp);
}
