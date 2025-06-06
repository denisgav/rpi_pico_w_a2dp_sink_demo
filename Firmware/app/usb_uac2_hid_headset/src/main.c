/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Jerzy Kasenberg
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

#include <stdio.h>
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

#define MAIN_TASK_PRIORITY (tskIDLE_PRIORITY + 2UL)
#define SPK_BLINK_TASK_PRIORITY (tskIDLE_PRIORITY + 0UL)
#define MIC_BLINK_TASK_PRIORITY (tskIDLE_PRIORITY + 0UL)
#define STATUS_UPDATE_TASK_PRIORITY (tskIDLE_PRIORITY + 1UL)
#define KEYPAD_TASK_PRIORITY (tskIDLE_PRIORITY + 1UL)
//-----------------------------

#include "bsp/board_api.h"
#include "tusb.h"

#include "main.h"
#include "board_defines.h"

#include "usb_descriptors.h"
#include "usb_headset_settings.h"
#include "usb_common_cb.h"
#include "usb_headset.h"
#include "usb_keypad.h"

#include "i2s/machine_i2s.h"
#include "volume_ctrl.h"

#include "ssd1306/ssd1306.h"

#include "keypad.h"

// Pointer to I2S handler
machine_i2s_obj_t *speaker_i2s0 = NULL;
machine_i2s_obj_t *microphone_i2s0 = NULL;

usb_headset_settings_t headset_settings;

// Buffer for speaker data
// i2s_32b_audio_sample spk_i2s_buffer[SAMPLE_BUFFER_SIZE];
i2s_32b_audio_sample spk_32b_i2s_buffer[SAMPLE_BUFFER_SIZE];
i2s_16b_audio_sample spk_16b_i2s_buffer[SAMPLE_BUFFER_SIZE];
int32_t spk_buf[CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ / 4];

// Buffer for microphone data
usb_audio_4b_sample mic_32b_i2s_buffer[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX * CFG_TUD_AUDIO_FUNC_1_MAX_SAMPLE_RATE / 1000];
usb_audio_2b_sample mic_16b_i2s_buffer[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX * CFG_TUD_AUDIO_FUNC_1_MAX_SAMPLE_RATE / 1000];

void spk_led_blinking_task(__unused void *params);
void mic_led_blinking_task(__unused void *params);
void status_update_task(__unused void *params);
void main_task(__unused void *params);

void keypad_buttons_handler(KeypadCode_e keyCode, KeypadEvent_e events);
void on_button_press(uint8_t keycode, KeypadEvent_e events);

void vLaunch(void);

int32_t usb_to_i2s_32b_sample_convert(int32_t sample, uint32_t volume_db);
int16_t usb_to_i2s_16b_sample_convert(int16_t sample, uint32_t volume_db);
int32_t i2s_to_usb_32b_sample_convert(int32_t sample, uint32_t volume_db);
int16_t i2s_to_usb_16b_sample_convert(int16_t sample, uint32_t volume_db);

void refresh_i2s_connections()
{
  headset_settings.spk_samples_in_i2s_frame_min = (headset_settings.spk_sample_rate) / 1000;
  headset_settings.spk_samples_in_i2s_frame_max = (headset_settings.spk_sample_rate + 999) / 1000;

  headset_settings.mic_samples_in_i2s_frame_min = (headset_settings.mic_sample_rate) / 1000;
  headset_settings.mic_samples_in_i2s_frame_max = (headset_settings.mic_sample_rate + 999) / 1000;

  speaker_i2s0 = create_machine_i2s(0, GPIO_I2S_SPK_SCK, GPIO_I2S_SPK_WS, GPIO_I2S_SPK_DATA, TX,
                                    ((headset_settings.spk_resolution == 16) ? 16 : 32), /*ringbuf_len*/ SIZEOF_DMA_BUFFER_IN_BYTES, headset_settings.spk_sample_rate);

  microphone_i2s0 = create_machine_i2s(1, GPIO_I2S_MIC_SCK, GPIO_I2S_MIC_WS, GPIO_I2S_MIC_DATA, RX,
                                       I2S_MIC_BPS, /*ringbuf_len*/ SIZEOF_DMA_BUFFER_IN_BYTES, headset_settings.mic_sample_rate);
}

void usb_current_status_set_handler(uint8_t itf, uint32_t blink_interval_ms_in);

void usb_headset_mute_handler(uint8_t itf, int8_t bChannelNumber, int8_t mute_in);
void usb_headset_volume_handler(uint8_t itf, int8_t bChannelNumber, int16_t volume_in);
void usb_headset_current_sample_rate_handler(uint8_t itf, uint32_t current_sample_rate_in);
void usb_headset_current_resolution_handler(uint8_t itf, uint8_t current_resolution_in);

void usb_headset_tud_audio_rx_done_pre_read_handler(uint8_t rhport, uint16_t n_bytes_received, uint8_t func_id, uint8_t ep_out, uint8_t cur_alt_setting);

volatile uint32_t num_of_mic_samples;
void usb_headset_tx_pre_load(uint8_t rhport, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting);
void usb_headset_tx_post_load(uint8_t rhport, uint16_t n_bytes_copied, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting);
uint32_t get_num_of_mic_samples();

uint16_t usb_keypad_get_report_handler(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen);
void usb_keypad_set_report_handler(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize);

//---------------------------------------
//           SSD1306
//---------------------------------------
ssd1306_t disp;
void setup_ssd1306();
void display_ssd1306_info();
//---------------------------------------

/*------------- MAIN -------------*/
int main(void)
{
  stdio_init_all();

  /* Configure the hardware ready to run the demo. */
  const char *rtos_name;
#if (configNUMBER_OF_CORES > 1)
  rtos_name = "FreeRTOS SMP";
#else
  rtos_name = "FreeRTOS";
#endif

#if (configNUMBER_OF_CORES == 2)
  printf("Starting %s on both cores:\n", rtos_name);
  vLaunch();
#elif (RUN_FREE_RTOS_ON_CORE == 1)
  printf("Starting %s on core 1:\n", rtos_name);
  multicore_launch_core1(vLaunch);
  while (true)
    ;
#else
  printf("Starting %s on core 0:\n", rtos_name);
  vLaunch();
#endif
  return 0;
}

void main_task(__unused void *params)
{
  num_of_mic_samples = 0;

  headset_settings.spk_sample_rate = I2S_SPK_RATE_DEF;
  headset_settings.spk_resolution = CFG_TUD_AUDIO_FUNC_1_FORMAT_1_RESOLUTION_RX;
  headset_settings.spk_blink_interval_ms = BLINK_NOT_MOUNTED;

  for (int i = 0; i < (CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX + 1); i++)
  {
    headset_settings.spk_volume[i] = DEFAULT_VOLUME;
    headset_settings.spk_mute[i] = 0;
    headset_settings.spk_volume_db[i] = vol_to_db_convert_enc(headset_settings.spk_mute[i], headset_settings.spk_volume[i]);
  }

  for (int i = 0; i < (CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX); i++)
  {
    headset_settings.spk_volume_mul_db[i] = headset_settings.spk_volume_db[0] * headset_settings.spk_volume_db[i + 1];
  }

  headset_settings.mic_sample_rate = I2S_SPK_RATE_DEF;
  headset_settings.mic_resolution = CFG_TUD_AUDIO_FUNC_1_FORMAT_1_RESOLUTION_TX;
  headset_settings.mic_blink_interval_ms = BLINK_NOT_MOUNTED;

  for (int i = 0; i < (CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1); i++)
  {
    headset_settings.mic_volume[i] = DEFAULT_VOLUME;
    headset_settings.mic_mute[i] = 0;
    headset_settings.mic_volume_db[i] = vol_to_db_convert_enc(headset_settings.mic_mute[i], headset_settings.mic_volume[i]);
  }

  for (int i = 0; i < (CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX); i++)
  {
    headset_settings.mic_volume_mul_db[i] = headset_settings.mic_volume_db[0] * headset_settings.mic_volume_db[i + 1];
  }

  headset_settings.status_updated = false;

  setup_ssd1306();

  usb_set_current_status_set_handler(usb_current_status_set_handler);

  usb_headset_set_mute_set_handler(usb_headset_mute_handler);
  usb_headset_set_volume_set_handler(usb_headset_volume_handler);
  usb_headset_set_current_sample_rate_set_handler(usb_headset_current_sample_rate_handler);
  usb_headset_set_current_resolution_set_handler(usb_headset_current_resolution_handler);

  usb_headset_set_tud_audio_rx_done_pre_read_set_handler(usb_headset_tud_audio_rx_done_pre_read_handler);

  usb_headset_set_tud_audio_tx_done_pre_load_set_handler(usb_headset_tx_pre_load);
  usb_headset_set_tud_audio_tx_done_post_load_set_handler(usb_headset_tx_post_load);

  usb_keypad_set_get_report_handler(usb_keypad_get_report_handler);
  usb_keypad_set_set_report_handler(usb_keypad_set_report_handler);

  usb_headset_init();
  refresh_i2s_connections();

  TaskHandle_t spk_led_blink_t;
  xTaskCreate(spk_led_blinking_task, "SPK_LED_BlinkingTask", 4096, NULL, SPK_BLINK_TASK_PRIORITY, &spk_led_blink_t);

  TaskHandle_t mic_led_blink_t;
  xTaskCreate(mic_led_blinking_task, "MIC_LED_BlinkingTask", 4096, NULL, MIC_BLINK_TASK_PRIORITY, &mic_led_blink_t);

  TaskHandle_t status_update_t;
  xTaskCreate(status_update_task, "StatusUpdateTask", 4096, NULL, STATUS_UPDATE_TASK_PRIORITY, &status_update_t);

  set_keypad_button_handler(keypad_buttons_handler);
  
  TaskHandle_t keypad_task;
  xTaskCreate(keypad_buttons_handle_task, "KeypadTask", 1024, NULL, KEYPAD_TASK_PRIORITY, &keypad_task);

  while (1)
  {
    usb_headset_task(); // tinyusb device task
    // vTaskDelay(0);
  }

#if HAVE_LWIP && !CYW43_LWIP
  lwip_freertos_deinit(cyw43_arch_async_context());
#endif
}

void vLaunch(void)
{
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
void setup_ssd1306()
{
  i2c_init(I2C_INST, 400000);
  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA);
  gpio_pull_up(I2C_SCL);

  disp.external_vcc = false;
  ssd1306_init(&disp, I2C_SSD1306_WIDTH, I2C_SSD1306_HEIGHT, I2C_SSD1306_ADDR, I2C_INST);
  ssd1306_clear(&disp);

  ssd1306_draw_string(&disp, 4, 16, 1, "Raspberry pi pico");
  ssd1306_draw_string(&disp, 8, 32, 1, "USB UAC2 speaker");
  ssd1306_show(&disp);
}
//-------------------------

//-------------------------
// keypad callback functions
//-------------------------
void keypad_buttons_handler(KeypadCode_e keyCode, KeypadEvent_e events)
{
  switch (keyCode)
  {
    case KEYPAD_BTN_0_0:
    {
      on_button_press(TUD_HID_MULTIMEDIA_CONSUMER_PLAY_PAUSE_CODE, events);
      break;
    }
    case KEYPAD_BTN_0_1:
    {
      on_button_press(TUD_HID_MULTIMEDIA_CONSUMER_STOP_CODE, events);
      break;
    }
    case KEYPAD_BTN_0_2:
    {
      on_button_press(TUD_HID_MULTIMEDIA_CONSUMER_SCAN_PREVIOUS_CODE, events);
      break;
    }
    case KEYPAD_BTN_0_3:
    {
      on_button_press(TUD_HID_MULTIMEDIA_CONSUMER_SCAN_NEXT_CODE, events);
      break;
    }

    case KEYPAD_BTN_1_0:
    {
      if (events == KEYPAD_EDGE_RISE)
      {
        headset_settings.usr_mic_mute = !headset_settings.usr_mic_mute;
      }
      break;
    }
    case KEYPAD_BTN_1_1:
    {
      on_button_press(TUD_HID_MULTIMEDIA_CONSUMER_MUTE_CODE, events);
      break;
    }
    case KEYPAD_BTN_1_2:
    {
      on_button_press(TUD_HID_MULTIMEDIA_CONSUMER_VOLUME_DECREMENT_CODE, events);
      break;
    }
    case KEYPAD_BTN_1_3:
    {
      on_button_press(TUD_HID_MULTIMEDIA_CONSUMER_VOLUME_INCREMENT_CODE, events);
      break;
    }
    default:
    {
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
  //if ( tud_hid_n_ready(ITF_NUM_HID1) )
  if ( tud_hid_ready() )
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

//-------------------------

//-------------------------
// callback functions
//-------------------------

void usb_headset_mute_handler(uint8_t itf, int8_t bChannelNumber, int8_t mute_in)
{
  if ((itf == 0) || (itf == ITF_NUM_AUDIO_STREAMING_SPK))
  {
    headset_settings.spk_mute[bChannelNumber] = mute_in;
    headset_settings.spk_volume_db[bChannelNumber] = vol_to_db_convert_enc(headset_settings.spk_mute[bChannelNumber], headset_settings.spk_volume[bChannelNumber]);

    for (int i = 0; i < (CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX); i++)
    {
      headset_settings.spk_volume_mul_db[i] = headset_settings.spk_volume_db[0] * headset_settings.spk_volume_db[i + 1];
    }
  }

  if ((itf == 0) || (itf == ITF_NUM_AUDIO_STREAMING_MIC))
  {
    headset_settings.mic_mute[bChannelNumber] = mute_in;
    headset_settings.mic_volume_db[bChannelNumber] = vol_to_db_convert_enc(headset_settings.mic_mute[bChannelNumber], headset_settings.mic_volume[bChannelNumber]);

    for (int i = 0; i < (CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX); i++)
    {
      headset_settings.mic_volume_mul_db[i] = headset_settings.mic_volume_db[0] * headset_settings.mic_volume_db[i + 1];
    }
  }

  headset_settings.status_updated = true;
}

void usb_headset_volume_handler(uint8_t itf, int8_t bChannelNumber, int16_t volume_in)
{
  if ((itf == 0) || (itf == ITF_NUM_AUDIO_STREAMING_SPK))
  {
    headset_settings.spk_volume[bChannelNumber] = volume_in;
    headset_settings.spk_volume_db[bChannelNumber] = vol_to_db_convert_enc(headset_settings.spk_mute[bChannelNumber], headset_settings.spk_volume[bChannelNumber]);

    for (int i = 0; i < (CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX); i++)
    {
      headset_settings.spk_volume_mul_db[i] = headset_settings.spk_volume_db[0] * headset_settings.spk_volume_db[i + 1];
    }
  }

  if ((itf == 0) || (itf == ITF_NUM_AUDIO_STREAMING_MIC))
  {
    headset_settings.mic_volume[bChannelNumber] = volume_in;
    headset_settings.mic_volume_db[bChannelNumber] = vol_to_db_convert_enc(headset_settings.mic_mute[bChannelNumber], headset_settings.mic_volume[bChannelNumber]);

    for (int i = 0; i < (CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX); i++)
    {
      headset_settings.mic_volume_mul_db[i] = headset_settings.mic_volume_db[0] * headset_settings.mic_volume_db[i + 1];
    }
  }

  headset_settings.status_updated = true;
}

void usb_headset_current_sample_rate_handler(uint8_t itf, uint32_t current_sample_rate_in)
{
  if ((itf == 0) || (itf == ITF_NUM_AUDIO_STREAMING_SPK))
    headset_settings.spk_sample_rate = current_sample_rate_in;
  if ((itf == 0) || (itf == ITF_NUM_AUDIO_STREAMING_MIC))
    headset_settings.mic_sample_rate = current_sample_rate_in;

  refresh_i2s_connections();
  headset_settings.status_updated = true;
}

void usb_headset_current_resolution_handler(uint8_t itf, uint8_t current_resolution_in)
{
  if ((itf == 0) || (itf == ITF_NUM_AUDIO_STREAMING_SPK))
    headset_settings.spk_resolution = current_resolution_in;
  if ((itf == 0) || (itf == ITF_NUM_AUDIO_STREAMING_MIC))
    headset_settings.mic_resolution = current_resolution_in;

  refresh_i2s_connections();
  headset_settings.status_updated = true;
}

void usb_current_status_set_handler(uint8_t itf, uint32_t blink_interval_ms_in)
{
  if ((itf == 0) || (itf == ITF_NUM_AUDIO_STREAMING_SPK))
    headset_settings.spk_blink_interval_ms = blink_interval_ms_in;
  if ((itf == 0) || (itf == ITF_NUM_AUDIO_STREAMING_MIC))
    headset_settings.mic_blink_interval_ms = blink_interval_ms_in;

  headset_settings.status_updated = true;
}

void usb_headset_tud_audio_rx_done_pre_read_handler(uint8_t rhport, uint16_t n_bytes_received, uint8_t func_id, uint8_t ep_out, uint8_t cur_alt_setting)
{
  uint32_t volume_db_left = headset_settings.spk_volume_mul_db[0];
  uint32_t volume_db_right = headset_settings.spk_volume_mul_db[1];

  if (speaker_i2s0 && (headset_settings.spk_blink_interval_ms == BLINK_STREAMING))
  {
    // Speaker data size received in the last frame
    uint16_t usb_spk_data_size = tud_audio_read(spk_buf, n_bytes_received);
    uint16_t usb_sample_count = 0;

    if (headset_settings.spk_resolution == 16)
    {
      int16_t *in = (int16_t *)spk_buf;
      usb_sample_count = usb_spk_data_size / 4; // 4 bytes per sample 2b left, 2b right

      if (usb_sample_count >= headset_settings.spk_samples_in_i2s_frame_min)
      {
        for (int i = 0; i < usb_sample_count; i++)
        {
          int16_t left = in[i * 2 + 0];
          int16_t right = in[i * 2 + 1];

          left = usb_to_i2s_16b_sample_convert(left, volume_db_left);
          right = usb_to_i2s_16b_sample_convert(right, volume_db_right);

          spk_16b_i2s_buffer[i].left = left;
          spk_16b_i2s_buffer[i].right = right;
        }
        machine_i2s_write_stream(speaker_i2s0, (void *)&spk_16b_i2s_buffer[0], usb_sample_count * 4);
      }
    }
    else
    {
      int32_t *in = (int32_t *)spk_buf;
      usb_sample_count = usb_spk_data_size / 8; // 8 bytes per sample 4b left, 4b right

      if (usb_sample_count >= headset_settings.spk_samples_in_i2s_frame_min)
      {
        for (int i = 0; i < usb_sample_count; i++)
        {
          int32_t left = in[i * 2 + 0];
          int32_t right = in[i * 2 + 1];

          left = usb_to_i2s_32b_sample_convert(left, volume_db_left);
          right = usb_to_i2s_32b_sample_convert(right, volume_db_right);

          spk_32b_i2s_buffer[i].left = left;
          spk_32b_i2s_buffer[i].right = right;
        }
        machine_i2s_write_stream(speaker_i2s0, (void *)&spk_32b_i2s_buffer[0], usb_sample_count * 8);
      }
    }
  }
}

void usb_headset_tx_pre_load(uint8_t rhport, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting)
{
  if (headset_settings.mic_resolution == 24)
  {
    // uint32_t buffer_size = headset_settings.samples_in_i2s_frame_min * CFG_TUD_AUDIO_FUNC_1_FORMAT_2_N_BYTES_PER_SAMPLE_TX * CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX;
    uint32_t buffer_size = num_of_mic_samples * CFG_TUD_AUDIO_FUNC_1_FORMAT_2_N_BYTES_PER_SAMPLE_TX * CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX;
    tud_audio_write(mic_32b_i2s_buffer, buffer_size);
  }
  else
  {
    // uint32_t buffer_size = headset_settings.samples_in_i2s_frame_min * CFG_TUD_AUDIO_FUNC_1_FORMAT_1_N_BYTES_PER_SAMPLE_TX * CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX;
    uint32_t buffer_size = num_of_mic_samples * CFG_TUD_AUDIO_FUNC_1_FORMAT_1_N_BYTES_PER_SAMPLE_TX * CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX;
    tud_audio_write(mic_16b_i2s_buffer, buffer_size);
  }
}

#if CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX == 2
void usb_headset_tx_post_load(uint8_t rhport, uint16_t n_bytes_copied, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting)
{
  if (microphone_i2s0)
  {
    i2s_32b_audio_sample buffer[USB_MIC_SAMPLE_BUFFER_SIZE];

    // Read data from microphone
    num_of_mic_samples = get_num_of_mic_samples();
    uint32_t buffer_size_read = num_of_mic_samples * (4 * 2);
    int num_bytes_read = machine_i2s_read_stream(microphone_i2s0, (void *)&buffer[0], buffer_size_read);

    uint32_t volume_db_left = headset_settings.mic_volume_mul_db[0];
    uint32_t volume_db_right = headset_settings.mic_volume_mul_db[1];
    {
      int num_of_frames_read = num_bytes_read / (4 * 2);
      for (uint32_t i = 0; i < num_of_frames_read; i++)
      {
        if (headset_settings.mic_resolution == 24)
        {
          int32_t left_24b = (int32_t)buffer[i].left << FORMAT_24B_TO_24B_SHIFT_VAL;   // Magic number
          int32_t right_24b = (int32_t)buffer[i].right << FORMAT_24B_TO_24B_SHIFT_VAL; // Magic number
          left_24b = i2s_to_usb_32b_sample_convert(left_24b, volume_db_left);
          right_24b = i2s_to_usb_32b_sample_convert(right_24b, volume_db_right);

          mic_32b_i2s_buffer[i * 2 + 0] = left_24b;  // TODO: check this value
          mic_32b_i2s_buffer[i * 2 + 1] = right_24b; // TODO: check this value
        }
        else
        {
          int32_t left_16b = (int32_t)buffer[i].left >> FORMAT_24B_TO_16B_SHIFT_VAL;   // Magic number
          int32_t right_16b = (int32_t)buffer[i].right >> FORMAT_24B_TO_16B_SHIFT_VAL; // Magic number
          left_16b = i2s_to_usb_16b_sample_convert(left_16b, volume_db_left);
          right_16b = i2s_to_usb_16b_sample_convert(right_16b, volume_db_right);

          mic_16b_i2s_buffer[i * 2 + 0] = left_16b;  // TODO: check this value
          mic_16b_i2s_buffer[i * 2 + 1] = right_16b; // TODO: check this value
        }
      }
    }
  }
}
#else  // CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX == 2
void usb_headset_tx_post_load(uint8_t rhport, uint16_t n_bytes_copied, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting)
{
  if (microphone_i2s0)
  {
    i2s_32b_audio_sample buffer[USB_MIC_SAMPLE_BUFFER_SIZE];

    // Read data from microphone
    num_of_mic_samples = get_num_of_mic_samples();
    uint32_t buffer_size_read = num_of_mic_samples * (4 * 2);
    int num_bytes_read = machine_i2s_read_stream(microphone_i2s0, (void *)&buffer[0], buffer_size_read);

    uint32_t volume_db_mono = headset_settings.mic_volume_mul_db[0];
    {
      int num_of_frames_read = num_bytes_read / (4 * 2);
      for (uint32_t i = 0; i < num_of_frames_read; i++)
      {
        if (headset_settings.resolution == 24)
        {
          int32_t mono_24b = (int32_t)buffer[i].left << FORMAT_24B_TO_24B_SHIFT_VAL; // Magic number
          mono_24b = i2s_to_usb_32b_sample_convert(mono_24b, volume_db_mono);

          mic_32b_i2s_buffer[i] = mono_24b; // TODO: check this value
        }
        else
        {
          int32_t mono_24b = (int32_t)buffer[i].left >> FORMAT_24B_TO_16B_SHIFT_VAL; // Magic number
          mono_24b = i2s_to_usb_16b_sample_convert(mono_24b, volume_db_mono);

          mic_16b_i2s_buffer[i] = mono_24b; // TODO: check this value
        }
      }
    }
  }
}
#endif // CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX == 2

uint32_t get_num_of_mic_samples()
{
  static uint32_t format_44100_khz_counter = 0;
  if (headset_settings.mic_sample_rate == 44100)
  {
    format_44100_khz_counter++;
    if (format_44100_khz_counter >= 9)
    {
      format_44100_khz_counter = 0;
      return 45;
    }
    else
    {
      return 44;
    }
  }
  else
  {
    return headset_settings.mic_samples_in_i2s_frame_min;
  }
}

uint16_t usb_keypad_get_report_handler(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
  // TODO not Implemented
  (void)itf;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;
  return 0;
}

void usb_keypad_set_report_handler(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
  // TODO set LED based on CAPLOCK, NUMLOCK etc...
  (void)itf;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)bufsize;
}

int32_t usb_to_i2s_32b_sample_convert(int32_t sample, uint32_t volume_db)
{
  int64_t sample_tmp = (int64_t)sample * (int64_t)volume_db;
  sample_tmp = sample_tmp >> (15 + 15);
  return (int32_t)sample_tmp;
  // return (int32_t)sample;
}

int16_t usb_to_i2s_16b_sample_convert(int16_t sample, uint32_t volume_db)
{
  int64_t sample_tmp = (int64_t)sample * (int64_t)volume_db;
  sample_tmp = sample_tmp >> (15 + 15);
  return (int16_t)sample_tmp;
  // return (int16_t)sample;
}

int32_t i2s_to_usb_32b_sample_convert(int32_t sample, uint32_t volume_db)
{
#ifdef APPLY_VOLUME_FEATURE
  if (headset_settings.usr_mic_mute)
    return 0;
  else
  {
    int64_t sample_tmp = (int64_t)sample * (int64_t)volume_db;
    sample_tmp = sample_tmp >> (15 + 15);
    return (int32_t)sample_tmp;
    // return (int32_t)sample;
  }
#else  // APPLY_VOLUME_FEATURE
  if (headset_settings.usr_mic_mute)
    return 0;
  else
  {
    return (int32_t)sample;
  }
#endif // APPLY_VOLUME_FEATURE
}

int16_t i2s_to_usb_16b_sample_convert(int16_t sample, uint32_t volume_db)
{
#ifdef APPLY_VOLUME_FEATURE
  if (headset_settings.usr_mic_mute)
    return 0;
  else
  {
    int64_t sample_tmp = (int64_t)(sample) * (int64_t)volume_db;
    sample_tmp = sample_tmp >> (15 + 15);
    return (int16_t)sample_tmp;
    // return (int16_t)sample;
  }
#else  // APPLY_VOLUME_FEATURE
  if (headset_settings.usr_mic_mute)
    return 0;
  else
  {
    return (int16_t)(sample);
  }
#endif // APPLY_VOLUME_FEATURE
}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void spk_led_blinking_task(__unused void *params)
{
  gpio_init(SPK_STATUS_LED);
  gpio_set_dir(SPK_STATUS_LED, GPIO_OUT);

  bool led_state = false;
  while (true)
  {
    uint32_t blink_interval_ms = headset_settings.spk_blink_interval_ms;
    vTaskDelay(blink_interval_ms);

    // board_led_write(led_state);
    gpio_put(SPK_STATUS_LED, led_state);
    led_state = 1 - led_state;
  }
}

void mic_led_blinking_task(__unused void *params)
{
  gpio_init(MIC_STATUS_LED);
  gpio_set_dir(MIC_STATUS_LED, GPIO_OUT);

  gpio_init(MIC_MUTE_LED);
  gpio_set_dir(MIC_MUTE_LED, GPIO_OUT);

  bool led_state = false;
  while (true)
  {
    uint32_t blink_interval_ms = headset_settings.mic_blink_interval_ms;
    vTaskDelay(blink_interval_ms);

    // board_led_write(led_state);
    gpio_put(MIC_STATUS_LED, led_state);
    led_state = 1 - led_state;

    gpio_put(MIC_MUTE_LED, headset_settings.usr_mic_mute);
  }
}

//--------------------------------------------------------------------+
// STATUS UPDATE TASK
//--------------------------------------------------------------------+
void status_update_task(__unused void *params)
{
  while (true)
  {
    vTaskDelay(500);

    if (headset_settings.status_updated == true)
    {
      headset_settings.status_updated = false;

      display_ssd1306_info();
    }
  }
}

void display_ssd1306_info(void)
{
  char fmt_tmp_str[20] = "";

  ssd1306_clear(&disp);

  switch (headset_settings.spk_blink_interval_ms)
  {
  case BLINK_NOT_MOUNTED:
  {
    ssd1306_draw_string(&disp, 4, 0, 1, "Spk not mounted");
    break;
  }
  case BLINK_SUSPENDED:
  {
    ssd1306_draw_string(&disp, 4, 0, 1, "Spk suspended");
    break;
  }
  case BLINK_MOUNTED:
  {
    ssd1306_draw_string(&disp, 4, 0, 1, "Spk mounted");
    break;
  }
  case BLINK_STREAMING:
  {
    char spk_streaming_str[20] = "Spk stream: ";
    memset(fmt_tmp_str, 0x0, sizeof(fmt_tmp_str));

    itoa(headset_settings.spk_resolution, fmt_tmp_str, 10);
    strcat(spk_streaming_str, fmt_tmp_str);
    strcat(spk_streaming_str, " bit");

    ssd1306_draw_string(&disp, 4, 0, 1, spk_streaming_str);
    break;
  }
  default:
  {
    ssd1306_draw_string(&disp, 4, 0, 1, "Spk unknown");
    break;
  }
  }

  {
    char freq_str[20] = "Spk Freq: ";

    memset(fmt_tmp_str, 0x0, sizeof(fmt_tmp_str));

    itoa((headset_settings.spk_sample_rate), fmt_tmp_str, 10);
    strcat(freq_str, fmt_tmp_str);
    strcat(freq_str, " Hz");

    ssd1306_draw_string(&disp, 4, 8, 1, freq_str);
  }

  switch (headset_settings.mic_blink_interval_ms)
  {
  case BLINK_NOT_MOUNTED:
  {
    ssd1306_draw_string(&disp, 4, 16, 1, "Mic not mounted");
    break;
  }
  case BLINK_SUSPENDED:
  {
    ssd1306_draw_string(&disp, 4, 16, 1, "Mic suspended");
    break;
  }
  case BLINK_MOUNTED:
  {
    ssd1306_draw_string(&disp, 4, 16, 1, "Mic mounted");
    break;
  }
  case BLINK_STREAMING:
  {
    char mic_streaming_str[20] = "Mic stream: ";
    memset(fmt_tmp_str, 0x0, sizeof(fmt_tmp_str));

    itoa(headset_settings.mic_resolution, fmt_tmp_str, 10);
    strcat(mic_streaming_str, fmt_tmp_str);
    strcat(mic_streaming_str, " bit");

    ssd1306_draw_string(&disp, 4, 16, 1, mic_streaming_str);
    break;
  }
  default:
  {
    ssd1306_draw_string(&disp, 4, 16, 1, "Mic unknown");
    break;
  }
  }

  {
    char freq_str[20] = "Mic Freq: ";

    memset(fmt_tmp_str, 0x0, sizeof(fmt_tmp_str));

    itoa((headset_settings.mic_sample_rate), fmt_tmp_str, 10);
    strcat(freq_str, fmt_tmp_str);
    strcat(freq_str, " Hz");

    ssd1306_draw_string(&disp, 4, 24, 1, freq_str);
  }

  {
    char spk_vol_m_str[20] = "Spk M:";
    char spk_vol_l_str[20] = "Spk L:";
    char spk_vol_r_str[20] = "Spk R:";

    char mic_vol_m_str[20] = "Mic M:";
    char mic_vol_l_str[20] = "Mic L:";
    char mic_vol_r_str[20] = "Mic R:";

    if (headset_settings.spk_mute[0])
    {
      strcat(spk_vol_m_str, "Mute");
    }
    else
    {
      memset(fmt_tmp_str, 0x0, sizeof(fmt_tmp_str));
      itoa((headset_settings.spk_volume[0] >> ENC_NUM_OF_FP_BITS),
           fmt_tmp_str, 10);
      strcat(spk_vol_m_str, fmt_tmp_str);
    }

    if (headset_settings.spk_mute[1])
    {
      strcat(spk_vol_l_str, "Mute");
    }
    else
    {
      memset(fmt_tmp_str, 0x0, sizeof(fmt_tmp_str));
      itoa((headset_settings.spk_volume[1] >> ENC_NUM_OF_FP_BITS),
           fmt_tmp_str, 10);
      strcat(spk_vol_l_str, fmt_tmp_str);
    }

    if (headset_settings.spk_mute[2])
    {
      strcat(spk_vol_r_str, "Mute");
    }
    else
    {
      memset(fmt_tmp_str, 0x0, sizeof(fmt_tmp_str));
      itoa((headset_settings.spk_volume[2] >> ENC_NUM_OF_FP_BITS),
           fmt_tmp_str, 10);
      strcat(spk_vol_r_str, fmt_tmp_str);
    }

    ssd1306_draw_string(&disp, 4, 32, 1, spk_vol_m_str);
    ssd1306_draw_string(&disp, 4, 40, 1, spk_vol_l_str);
    ssd1306_draw_string(&disp, 4, 48, 1, spk_vol_r_str);

    if (headset_settings.mic_mute[0])
    {
      strcat(mic_vol_m_str, "Mute");
    }
    else
    {
      memset(fmt_tmp_str, 0x0, sizeof(fmt_tmp_str));
      itoa((headset_settings.mic_volume[0] >> ENC_NUM_OF_FP_BITS),
           fmt_tmp_str, 10);
      strcat(mic_vol_m_str, fmt_tmp_str);
    }

    if (headset_settings.mic_mute[1])
    {
      strcat(mic_vol_l_str, "Mute");
    }
    else
    {
      memset(fmt_tmp_str, 0x0, sizeof(fmt_tmp_str));
      itoa((headset_settings.mic_volume[1] >> ENC_NUM_OF_FP_BITS),
           fmt_tmp_str, 10);
      strcat(mic_vol_l_str, fmt_tmp_str);
    }

    if (headset_settings.mic_mute[2])
    {
      strcat(mic_vol_r_str, "Mute");
    }
    else
    {
      memset(fmt_tmp_str, 0x0, sizeof(fmt_tmp_str));
      itoa((headset_settings.mic_volume[2] >> ENC_NUM_OF_FP_BITS),
           fmt_tmp_str, 10);
      strcat(mic_vol_r_str, fmt_tmp_str);
    }

    ssd1306_draw_string(&disp, 68, 32, 1, mic_vol_m_str);
    ssd1306_draw_string(&disp, 68, 40, 1, mic_vol_l_str);
    ssd1306_draw_string(&disp, 68, 48, 1, mic_vol_r_str);
  }
  ssd1306_show(&disp);
}
