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

#define MAIN_TASK_PRIORITY           ( tskIDLE_PRIORITY + 2UL )
#define BLINK_TASK_PRIORITY          ( tskIDLE_PRIORITY + 0UL )
#define STATUS_UPDATE_TASK_PRIORITY  ( tskIDLE_PRIORITY + 1UL )
//-----------------------------

#include "main.h"
#include "board_defines.h"

#include "usb_speaker.h"

#include "i2s/machine_i2s.h"
#include "volume_ctrl.h"

#include "ssd1306/ssd1306.h"


// Pointer to I2S handler
machine_i2s_obj_t* speaker_i2s0 = NULL;

speaker_settings_t speaker_settings;

// Buffer for speaker data
//i2s_32b_audio_sample spk_i2s_buffer[SAMPLE_BUFFER_SIZE];
i2s_32b_audio_sample spk_32b_i2s_buffer[SAMPLE_BUFFER_SIZE];
i2s_16b_audio_sample spk_16b_i2s_buffer[SAMPLE_BUFFER_SIZE];
int32_t spk_buf[CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ / 4];

void led_blinking_task(__unused void *params);
void status_update_task(__unused void *params);
void main_task(__unused void *params);

void vLaunch( void);

int32_t usb_to_i2s_32b_sample_convert(int32_t sample, uint32_t volume_db);

int16_t usb_to_i2s_16b_sample_convert(int16_t sample, uint32_t volume_db);

void refresh_i2s_connections()
{
  speaker_settings.samples_in_i2s_frame_min = (speaker_settings.sample_rate)    /1000;
  speaker_settings.samples_in_i2s_frame_max = (speaker_settings.sample_rate+999)/1000;

  speaker_i2s0 = create_machine_i2s(0, GPIO_I2S_SPK_SCK, GPIO_I2S_SPK_WS, GPIO_I2S_SPK_DATA, TX, 
    ((speaker_settings.resolution == 16) ? 16 : 32), /*ringbuf_len*/SIZEOF_DMA_BUFFER_IN_BYTES, speaker_settings.sample_rate);
  
  // update_pio_frequency(speaker_i2s0, speaker_settings.usb_sample_rate);
}


void usb_speaker_mute_handler(int8_t bChannelNumber, int8_t mute_in);
void usb_speaker_volume_handler(int8_t bChannelNumber, int16_t volume_in);
void usb_speaker_current_sample_rate_handler(uint32_t current_sample_rate_in);
void usb_speaker_current_resolution_handler(uint8_t current_resolution_in);
void usb_speaker_current_status_set_handler(uint32_t blink_interval_ms_in);

void usb_speaker_tud_audio_rx_done_pre_read_handler(uint8_t rhport, uint16_t n_bytes_received, uint8_t func_id, uint8_t ep_out, uint8_t cur_alt_setting);


//---------------------------------------
//           SSD1306
//---------------------------------------
ssd1306_t disp;
void setup_ssd1306();
void display_ssd1306_info();
//---------------------------------------

/*------------- MAIN -------------*/
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
  speaker_settings.sample_rate  = I2S_SPK_RATE_DEF;
  speaker_settings.resolution = CFG_TUD_AUDIO_FUNC_1_FORMAT_1_RESOLUTION_RX;
  speaker_settings.blink_interval_ms = BLINK_NOT_MOUNTED;
  speaker_settings.status_updated = false;

  setup_ssd1306();

  usb_speaker_set_mute_set_handler(usb_speaker_mute_handler);
  usb_speaker_set_volume_set_handler(usb_speaker_volume_handler);
  usb_speaker_set_current_sample_rate_set_handler(usb_speaker_current_sample_rate_handler);
  usb_speaker_set_current_resolution_set_handler(usb_speaker_current_resolution_handler);
  usb_speaker_set_current_status_set_handler(usb_speaker_current_status_set_handler);

  usb_speaker_set_tud_audio_rx_done_pre_read_set_handler(usb_speaker_tud_audio_rx_done_pre_read_handler);
  
  usb_speaker_init();
  refresh_i2s_connections();

  for(int i=0; i<(CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX + 1); i++) {
    speaker_settings.volume[i] = DEFAULT_VOLUME;
    speaker_settings.mute[i] = 0;
    speaker_settings.volume_db[i] = vol_to_db_convert_enc(speaker_settings.mute[i], speaker_settings.volume[i]);
  }

  for(int i=0; i<(CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX); i++) {
    speaker_settings.volume_mul_db[i] = speaker_settings.volume_db[0]
      * speaker_settings.volume_db[i+1];
  }

   TaskHandle_t led_blink_t;
  xTaskCreate(led_blinking_task, "LED_BlinkingTask", 4096, NULL, BLINK_TASK_PRIORITY, &led_blink_t);

   TaskHandle_t status_update_t;
  xTaskCreate(status_update_task, "StatusUpdateTask", 4096, NULL, STATUS_UPDATE_TASK_PRIORITY, &status_update_t);

  while (1){
    usb_speaker_task(); // tinyusb device task
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

  ssd1306_draw_string(&disp, 4, 16, 1, "Raspberry pi pico");
  ssd1306_draw_string(&disp, 8, 32, 1, "USB UAC2 speaker");
  ssd1306_show(&disp);
}
//-------------------------

//-------------------------

//-------------------------
// callback functions
//-------------------------


void usb_speaker_mute_handler(int8_t bChannelNumber, int8_t mute_in)
{
  speaker_settings.mute[bChannelNumber] = mute_in;
  speaker_settings.volume_db[bChannelNumber] = vol_to_db_convert_enc(speaker_settings.mute[bChannelNumber], speaker_settings.volume[bChannelNumber]);

  for(int i=0; i<(CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX); i++) {
    speaker_settings.volume_mul_db[i] = speaker_settings.volume_db[0]
      * speaker_settings.volume_db[i+1];
  }

  speaker_settings.status_updated = true;
}

void usb_speaker_volume_handler(int8_t bChannelNumber, int16_t volume_in)
{
  speaker_settings.volume[bChannelNumber] = volume_in;
  speaker_settings.volume_db[bChannelNumber] = vol_to_db_convert_enc(speaker_settings.mute[bChannelNumber], speaker_settings.volume[bChannelNumber]);

  for(int i=0; i<(CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX); i++) {
    speaker_settings.volume_mul_db[i] = speaker_settings.volume_db[0]
      * speaker_settings.volume_db[i+1];
  }

  speaker_settings.status_updated = true;
}

void usb_speaker_current_sample_rate_handler(uint32_t current_sample_rate_in)
{
  speaker_settings.sample_rate = current_sample_rate_in;
  refresh_i2s_connections();
  speaker_settings.status_updated = true;
}

void usb_speaker_current_resolution_handler(uint8_t current_resolution_in)
{
  speaker_settings.resolution = current_resolution_in;
  refresh_i2s_connections();
  speaker_settings.status_updated = true;
}

void usb_speaker_current_status_set_handler(uint32_t blink_interval_ms_in)
{
  speaker_settings.blink_interval_ms = blink_interval_ms_in;
  speaker_settings.status_updated = true;
}

void usb_speaker_tud_audio_rx_done_pre_read_handler(uint8_t rhport, uint16_t n_bytes_received, uint8_t func_id, uint8_t ep_out, uint8_t cur_alt_setting)
{
  uint32_t volume_db_left = speaker_settings.volume_mul_db[0];
  uint32_t volume_db_right = speaker_settings.volume_mul_db[1];

  if(speaker_i2s0 && (speaker_settings.blink_interval_ms == BLINK_STREAMING)){
    // Speaker data size received in the last frame
    uint16_t usb_spk_data_size = tud_audio_read(spk_buf, n_bytes_received);
    uint16_t usb_sample_count = 0;

    if (speaker_settings.resolution == 16)
    {
      int16_t *in = (int16_t *) spk_buf;
      usb_sample_count = usb_spk_data_size/4; // 4 bytes per sample 2b left, 2b right

      if(usb_sample_count >= speaker_settings.samples_in_i2s_frame_min){
        for (int i = 0; i < usb_sample_count; i++) {
          int16_t left = in[i*2 + 0];
          int16_t right = in[i*2 + 1];

          left = usb_to_i2s_16b_sample_convert(left, volume_db_left);
          right = usb_to_i2s_16b_sample_convert(right, volume_db_right);

          spk_16b_i2s_buffer[i].left  = left;
          spk_16b_i2s_buffer[i].right = right;
        }
        machine_i2s_write_stream(speaker_i2s0, (void*)&spk_16b_i2s_buffer[0], usb_sample_count*4);
      }
    }
    else 
    {
      int32_t *in = (int32_t *) spk_buf;
      usb_sample_count = usb_spk_data_size/8; // 8 bytes per sample 4b left, 4b right

      if(usb_sample_count >= speaker_settings.samples_in_i2s_frame_min){
        for (int i = 0; i < usb_sample_count; i++) {
          int32_t left = in[i*2 + 0];
          int32_t right = in[i*2 + 1];

          left = usb_to_i2s_32b_sample_convert(left, volume_db_left);
          right = usb_to_i2s_32b_sample_convert(right, volume_db_right);

          spk_32b_i2s_buffer[i].left  = left;
          spk_32b_i2s_buffer[i].right = right;
        }
        machine_i2s_write_stream(speaker_i2s0, (void*)&spk_32b_i2s_buffer[0], usb_sample_count*8);
      }
    }
  }
}


int32_t usb_to_i2s_32b_sample_convert(int32_t sample, uint32_t volume_db)
{
  int64_t sample_tmp = (int64_t)sample * (int64_t)volume_db;
  sample_tmp = sample_tmp>>(15+15);
  return (int32_t)sample_tmp;
  //return (int32_t)sample;
}

int16_t usb_to_i2s_16b_sample_convert(int16_t sample, uint32_t volume_db)
{
  int64_t sample_tmp = (int64_t)sample * (int64_t)volume_db;
  sample_tmp = sample_tmp>>(15+15);
  return (int16_t)sample_tmp;
  //return (int16_t)sample;
}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinking_task(__unused void *params){
  gpio_init(SPK_STATUS_LED);
  gpio_set_dir(SPK_STATUS_LED, GPIO_OUT);

  bool led_state = false;
  while(true){
    uint32_t blink_interval_ms = speaker_settings.blink_interval_ms;
    vTaskDelay(blink_interval_ms);

    //board_led_write(led_state);
    gpio_put(SPK_STATUS_LED, led_state);
    led_state = 1 - led_state;
  }
}

//--------------------------------------------------------------------+
// STATUS UPDATE TASK
//--------------------------------------------------------------------+
void status_update_task(__unused void *params){
  while(true){
    vTaskDelay(500);

    if(speaker_settings.status_updated == true){
      speaker_settings.status_updated = false;

      display_ssd1306_info();
    }    
  }
}

void display_ssd1306_info(void) {
  char fmt_tmp_str[20] = "";

  ssd1306_clear(&disp);

  switch (speaker_settings.blink_interval_ms) {
    case BLINK_NOT_MOUNTED: {
      ssd1306_draw_string(&disp, 4, 0, 1, "Spk not mounted");
      break;
    }
    case BLINK_SUSPENDED: {
      ssd1306_draw_string(&disp, 4, 0, 1, "Spk suspended");
      break;
    }
    case BLINK_MOUNTED: {
      ssd1306_draw_string(&disp, 4, 0, 1, "Spk mounted");
      break;
    }
    case BLINK_STREAMING: {
      char spk_streaming_str[20] = "Spk stream: ";
      memset(fmt_tmp_str, 0x0, sizeof(fmt_tmp_str));

      itoa(speaker_settings.resolution, fmt_tmp_str, 10);
      strcat(spk_streaming_str, fmt_tmp_str);
      strcat(spk_streaming_str, " bit");

      ssd1306_draw_string(&disp, 4, 0, 1, spk_streaming_str);
      break;
    }
    default: {
      ssd1306_draw_string(&disp, 4, 0, 1, "Spk unknown");
      break;
    }
  }

  

  {
    char freq_str[20] = "Freq: ";

    memset(fmt_tmp_str, 0x0, sizeof(fmt_tmp_str));

    itoa((speaker_settings.sample_rate), fmt_tmp_str, 10);
    strcat(freq_str, fmt_tmp_str);
    strcat(freq_str, " Hz");

    ssd1306_draw_string(&disp, 4, 16, 1, freq_str);
  }

  {
    char vol_m_str[20] = "Vol M:";
    char vol_l_str[20] = "Vol L:";
    char vol_r_str[20] = "Vol R:";

    char mute_m_str[20] = "Mute M:";
    char mute_l_str[20] = "Mute L:";
    char mute_r_str[20] = "Mute R:";

    memset(fmt_tmp_str, 0x0, sizeof(fmt_tmp_str));
    itoa((speaker_settings.volume[0] >> ENC_NUM_OF_FP_BITS),
        fmt_tmp_str, 10);
    strcat(vol_m_str, fmt_tmp_str);

    memset(fmt_tmp_str, 0x0, sizeof(fmt_tmp_str));
    itoa((speaker_settings.volume[1] >> ENC_NUM_OF_FP_BITS),
        fmt_tmp_str, 10);
    strcat(vol_l_str, fmt_tmp_str);

    memset(fmt_tmp_str, 0x0, sizeof(fmt_tmp_str));
    itoa((speaker_settings.volume[2] >> ENC_NUM_OF_FP_BITS),
        fmt_tmp_str, 10);
    strcat(vol_r_str, fmt_tmp_str);

    ssd1306_draw_string(&disp, 4, 24, 1, vol_m_str);
    ssd1306_draw_string(&disp, 4, 32, 1, vol_l_str);
    ssd1306_draw_string(&disp, 4, 40, 1, vol_r_str);

    strcat(mute_m_str, (speaker_settings.mute[0] ? "T" : "F"));
    strcat(mute_l_str, (speaker_settings.mute[1] ? "T" : "F"));
    strcat(mute_r_str, (speaker_settings.mute[2] ? "T" : "F"));

    ssd1306_draw_string(&disp, 68, 24, 1, mute_m_str);
    ssd1306_draw_string(&disp, 68, 32, 1, mute_l_str);
    ssd1306_draw_string(&disp, 68, 40, 1, mute_r_str);

  }
  ssd1306_show(&disp);
}
