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
#define STATUS_UPDATE_TASK_PRIORITY  ( tskIDLE_PRIORITY + 1UL )
//-----------------------------

#include "bsp/board_api.h"
#include "tusb.h"

#include "usb_descriptors.h"

#include "main.h"
#include "keypad_settings.h"
#include "usb_keypad.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+

keypad_settings_t keypad_settings;

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

void     usb_keypad_current_status_set_handler(uint32_t blink_interval_ms);
uint16_t usb_keypad_get_report_handler(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen);
void     usb_keypad_set_report_handler(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize);

int main(void){
  stdio_init_all();

  keypad_settings.blink_interval_ms = BLINK_NOT_MOUNTED;

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
  usb_keypad_set_current_status_set_handler(usb_keypad_current_status_set_handler);
  usb_keypad_set_get_report_handler(usb_keypad_get_report_handler);
  usb_keypad_set_set_report_handler(usb_keypad_set_report_handler);

  usb_keypad_init();

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
    usb_keypad_task();
    //vTaskDelay(0);
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
void     usb_keypad_current_status_set_handler(uint32_t blink_interval_ms){
  keypad_settings.blink_interval_ms = blink_interval_ms;
  status_updated = true;
}

uint16_t usb_keypad_get_report_handler(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen){
  // TODO not Implemented
  (void) itf;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) reqlen;
  return 0;
}

void     usb_keypad_set_report_handler(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize){
  // TODO set LED based on CAPLOCK, NUMLOCK etc...
  (void) itf;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) bufsize;
}

//-------------------------
// keypad callback functions
//-------------------------
void keypad_buttons_handler(KeypadCode_e keyCode, KeypadEvent_e events){
  switch(keyCode){
    case KEYPAD_BTN_0_0:{
      on_button_press(TUD_HID_MULTIMEDIA_CONSUMER_PLAY_PAUSE_CODE, events);
      break;
    }
    case KEYPAD_BTN_0_1:{
      on_button_press(TUD_HID_MULTIMEDIA_CONSUMER_STOP_CODE, events);
      break;
    }
    case KEYPAD_BTN_0_2:{
      on_button_press(TUD_HID_MULTIMEDIA_CONSUMER_SCAN_PREVIOUS_CODE, events);
      break;
    }
    case KEYPAD_BTN_0_3:{
      on_button_press(TUD_HID_MULTIMEDIA_CONSUMER_SCAN_NEXT_CODE, events);
      break;
    }


    case KEYPAD_BTN_1_0:{
      on_button_press(TUD_HID_MULTIMEDIA_CONSUMER_MUTE_CODE, events);
      break;
    }
    case KEYPAD_BTN_1_1:{
      on_button_press(TUD_HID_MULTIMEDIA_CONSUMER_VOLUME_DECREMENT_CODE, events);
      break;
    }
    case KEYPAD_BTN_1_2:{
      on_button_press(TUD_HID_MULTIMEDIA_CONSUMER_VOLUME_INCREMENT_CODE, events);
      break;
    }
    case KEYPAD_BTN_1_3:{
      on_button_press(TUD_HID_MULTIMEDIA_CONSUMER_VOLUME_CODE, events);
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
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinking_task(__unused void *params){
  gpio_init(GPIO_LED_0);
  gpio_set_dir(GPIO_LED_0, GPIO_OUT);

  bool led_state = false;
  while(true){
    vTaskDelay(keypad_settings.blink_interval_ms);

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

  switch (keypad_settings.blink_interval_ms) {
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
