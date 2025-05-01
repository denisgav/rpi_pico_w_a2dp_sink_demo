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

#if HAVE_LWIP
#include "pico/lwip_freertos.h"
#include "pico/cyw43_arch.h"
#endif

#include "FreeRTOS.h"
#include "task.h"

#include "usb_microphone.h"

#include "main.h"
#include "i2s/machine_i2s.h"
#include "volume_ctrl.h"

#include "ssd1306/ssd1306.h"

#include "keypad.h"

#ifndef RUN_FREERTOS_ON_CORE
#define RUN_FREERTOS_ON_CORE 0
#endif

#define MAIN_TASK_PRIORITY           ( tskIDLE_PRIORITY + 2UL )
#define BLINK_TASK_PRIORITY          ( tskIDLE_PRIORITY + 0UL )
#define STATUS_UPDATE_TASK_PRIORITY  ( tskIDLE_PRIORITY + 1UL )
#define KEYPAD_TASK_PRIORITY         ( tskIDLE_PRIORITY + 1UL )

// Pointer to I2S handler
machine_i2s_obj_t* microphone_i2s0 = NULL;

microphone_settings_t microphone_settings;


// Audio test data, 2 channels muxed together, buffer[0] for CH0, buffer[1] for CH1
usb_audio_4b_sample mic_32b_i2s_buffer[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX*CFG_TUD_AUDIO_FUNC_1_MAX_SAMPLE_RATE/1000];
usb_audio_2b_sample mic_16b_i2s_buffer[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX*CFG_TUD_AUDIO_FUNC_1_MAX_SAMPLE_RATE/1000];

//-------------------------
// callback functions
//-------------------------
void usb_microphone_mute_handler(int8_t bChannelNumber, int8_t mute_in);
void usb_microphone_volume_handler(int8_t bChannelNumber, int16_t volume_in);
void usb_microphone_current_sample_rate_handler(uint32_t current_sample_rate_in);
void usb_microphone_current_resolution_handler(uint8_t current_resolution_in);
void usb_microphone_current_status_set_handler(uint32_t blink_interval_ms_in);

uint32_t num_of_mic_samples;
void on_usb_microphone_tx_pre_load(uint8_t rhport, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting);
void on_usb_microphone_tx_post_load(uint8_t rhport, uint16_t n_bytes_copied, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting);
uint32_t get_num_of_mic_samples();
//-------------------------

void led_blinking_task(__unused void *params);
void status_update_task(__unused void *params);
void main_task(__unused void *params);

void keypad_buttons_handler(KeypadCode_e keyCode, KeypadEvent_e events);

void vLaunch( void);

void refresh_i2s_connections()
{
  microphone_settings.samples_in_i2s_frame_min = (microphone_settings.sample_rate)    /1000;
  microphone_settings.samples_in_i2s_frame_max = (microphone_settings.sample_rate+999)/1000;

  microphone_i2s0 = create_machine_i2s(0, GPIO_I2S_MIC_SCK, GPIO_I2S_MIC_WS, GPIO_I2S_MIC_DATA, RX, I2S_MIC_BPS, /*ringbuf_len*/SIZEOF_DMA_BUFFER_IN_BYTES, I2S_MIC_RATE_DEF);
}


//---------------------------------------
//           SSD1306
//---------------------------------------
ssd1306_t disp;
void setup_ssd1306();
void display_ssd1306_info();
//---------------------------------------

int32_t i2s_to_usb_32b_sample_convert(int32_t sample, uint32_t volume_db);
int16_t i2s_to_usb_16b_sample_convert(int16_t sample, uint32_t volume_db);

int main(void){
  stdio_init_all();

  num_of_mic_samples = 0;

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
  microphone_settings.sample_rate  = I2S_MIC_RATE_DEF;
  microphone_settings.resolution = CFG_TUD_AUDIO_FUNC_1_FORMAT_1_RESOLUTION_RX;
  microphone_settings.blink_interval_ms = BLINK_NOT_MOUNTED;
  microphone_settings.status_updated = false;
  microphone_settings.user_mute = false;

  setup_ssd1306();

  usb_microphone_set_mute_set_handler(usb_microphone_mute_handler);
  usb_microphone_set_volume_set_handler(usb_microphone_volume_handler);
  usb_microphone_set_current_sample_rate_set_handler(usb_microphone_current_sample_rate_handler);
  usb_microphone_set_current_resolution_set_handler(usb_microphone_current_resolution_handler);
  usb_microphone_set_current_status_set_handler(usb_microphone_current_status_set_handler);

  usb_microphone_set_tx_pre_load_handler(on_usb_microphone_tx_pre_load);
  usb_microphone_set_tx_post_load_handler(on_usb_microphone_tx_post_load);

  usb_microphone_init();
  refresh_i2s_connections();

  for(int i=0; i<(CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1); i++)
  {
    microphone_settings.volume[i] = DEFAULT_VOLUME;
    microphone_settings.mute[i] = 0;
    microphone_settings.volume_db[i] = vol_to_db_convert(microphone_settings.mute[i], microphone_settings.volume[i]);
  }

  for(int i=0; i<(CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX); i++) {
    microphone_settings.volume_mul_db[0] = microphone_settings.volume_db[0]
      * microphone_settings.volume_db[i+1];
  }

  TaskHandle_t led_blink_t;
  xTaskCreate(led_blinking_task, "LED_BlinkingTask", 4096, NULL, BLINK_TASK_PRIORITY, &led_blink_t);

  TaskHandle_t status_update_t;
  xTaskCreate(status_update_task, "StatusUpdateTask", 4096, NULL, STATUS_UPDATE_TASK_PRIORITY, &status_update_t);

  set_keypad_button_handler(keypad_buttons_handler);

  TaskHandle_t keypad_task;
  xTaskCreate(keypad_buttons_handle_task, "KeypadTask", 1024, NULL, KEYPAD_TASK_PRIORITY, &keypad_task);

  while (1){
    usb_microphone_task(); // tinyusb device task
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
  ssd1306_draw_string(&disp, 4, 16, 1, "USB UAC2 microphone");
  ssd1306_show(&disp);
}

//-------------------------

//-------------------------
// keypad callback functions
//-------------------------
void keypad_buttons_handler(KeypadCode_e keyCode, KeypadEvent_e events){
  switch(keyCode){
    case KEYPAD_BTN_0_0:{
      if(events == KEYPAD_EDGE_RISE){
        microphone_settings.user_mute = !microphone_settings.user_mute;
      }
      break;
    }
    default:{
      break;
    }
  }
}

//-------------------------
// callback functions
//-------------------------

void usb_microphone_mute_handler(int8_t bChannelNumber, int8_t mute_in)
{
  microphone_settings.mute[bChannelNumber] = mute_in;
  microphone_settings.volume_db[bChannelNumber] = vol_to_db_convert(microphone_settings.mute[bChannelNumber], microphone_settings.volume[bChannelNumber]);

  for(int i=0; i<(CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX); i++) {
    microphone_settings.volume_mul_db[i] = microphone_settings.volume_db[0]
      * microphone_settings.volume_db[i+1];
  }

  microphone_settings.status_updated = true;
}

void usb_microphone_volume_handler(int8_t bChannelNumber, int16_t volume_in)
{
  // If value in range -91 to 0, apply as is
  if((volume_in >= -91) && (volume_in <= 0))
    microphone_settings.volume[bChannelNumber] = volume_in;
   else { // Need to convert the value
     int16_t volume_tmp = volume_in >> ENC_NUM_OF_FP_BITS; // Value in range -128 to 127
     volume_tmp = volume_tmp - 127; // Value in range -255 to 0. Need to have -91 to 0
     volume_tmp = (volume_tmp*91)/255;
     microphone_settings.volume[bChannelNumber] = volume_tmp;    
  }
  microphone_settings.volume_db[bChannelNumber] = vol_to_db_convert(microphone_settings.mute[bChannelNumber], microphone_settings.volume[bChannelNumber]);

  for(int i=0; i<(CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX); i++) {
    microphone_settings.volume_mul_db[i] = microphone_settings.volume_db[0]
      * microphone_settings.volume_db[i+1];
  }

  microphone_settings.status_updated = true;
}

void usb_microphone_current_sample_rate_handler(uint32_t current_sample_rate_in)
{
  microphone_settings.sample_rate = current_sample_rate_in;
  refresh_i2s_connections();
  microphone_settings.status_updated = true;
}

void usb_microphone_current_resolution_handler(uint8_t current_resolution_in)
{
  microphone_settings.resolution = current_resolution_in;
  refresh_i2s_connections();
  microphone_settings.status_updated = true;
}

void usb_microphone_current_status_set_handler(uint32_t blink_interval_ms_in)
{
  microphone_settings.blink_interval_ms = blink_interval_ms_in;
  microphone_settings.status_updated = true;
}

void on_usb_microphone_tx_pre_load(uint8_t rhport, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting)
{
  if(microphone_settings.resolution == 24){
    //uint32_t buffer_size = microphone_settings.samples_in_i2s_frame_min * CFG_TUD_AUDIO_FUNC_1_FORMAT_2_N_BYTES_PER_SAMPLE_TX * CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX;
    uint32_t buffer_size = num_of_mic_samples * CFG_TUD_AUDIO_FUNC_1_FORMAT_2_N_BYTES_PER_SAMPLE_TX * CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX;
    tud_audio_write(mic_32b_i2s_buffer, buffer_size);
  } else {
    //uint32_t buffer_size = microphone_settings.samples_in_i2s_frame_min * CFG_TUD_AUDIO_FUNC_1_FORMAT_1_N_BYTES_PER_SAMPLE_TX * CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX;
    uint32_t buffer_size = num_of_mic_samples * CFG_TUD_AUDIO_FUNC_1_FORMAT_1_N_BYTES_PER_SAMPLE_TX * CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX;
    tud_audio_write(mic_16b_i2s_buffer, buffer_size);
  }
}

#if CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX == 2
void on_usb_microphone_tx_post_load(uint8_t rhport, uint16_t n_bytes_copied, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting)
{
  if(microphone_i2s0) {
    i2s_32b_audio_sample buffer[USB_MIC_SAMPLE_BUFFER_SIZE];

    // Read data from microphone
    num_of_mic_samples = get_num_of_mic_samples();
    uint32_t buffer_size_read = num_of_mic_samples * (4 * 2);
    int num_bytes_read = machine_i2s_read_stream(microphone_i2s0, (void*)&buffer[0], buffer_size_read);

    uint32_t volume_db_left = microphone_settings.volume_mul_db[0];
    uint32_t volume_db_right = microphone_settings.volume_mul_db[1];
    {
      int num_of_frames_read = num_bytes_read/(4 * 2);
      for(uint32_t i = 0; i < num_of_frames_read; i++){
        if(microphone_settings.resolution == 24){
          int32_t left_24b = (int32_t)buffer[i].left << FORMAT_24B_TO_24B_SHIFT_VAL; // Magic number
          int32_t right_24b = (int32_t)buffer[i].right << FORMAT_24B_TO_24B_SHIFT_VAL; // Magic number
          left_24b = i2s_to_usb_32b_sample_convert(left_24b, volume_db_left);
          right_24b = i2s_to_usb_32b_sample_convert(right_24b, volume_db_right);

          mic_32b_i2s_buffer[i*2] = left_24b; // TODO: check this value
          mic_32b_i2s_buffer[i*2+1] = right_24b; // TODO: check this value
        }
        else {
          int32_t left_16b = (int32_t)buffer[i].left >> FORMAT_24B_TO_16B_SHIFT_VAL; // Magic number
          int32_t right_16b = (int32_t)buffer[i].right >> FORMAT_24B_TO_16B_SHIFT_VAL; // Magic number
          left_16b = i2s_to_usb_16b_sample_convert(left_16b, volume_db_left);
          right_16b = i2s_to_usb_16b_sample_convert(right_16b, volume_db_right);

          mic_16b_i2s_buffer[i*2]   = left_16b; // TODO: check this value
          mic_16b_i2s_buffer[i*2+1] = right_16b; // TODO: check this value
        }
      }
    }
  }
}
#else // CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX == 2
void on_usb_microphone_tx_post_load(uint8_t rhport, uint16_t n_bytes_copied, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting)
{
  if(microphone_i2s0) {
    i2s_32b_audio_sample buffer[USB_MIC_SAMPLE_BUFFER_SIZE];

    // Read data from microphone
    num_of_mic_samples = get_num_of_mic_samples();
    uint32_t buffer_size_read = num_of_mic_samples * (4 * 2);
    int num_bytes_read = machine_i2s_read_stream(microphone_i2s0, (void*)&buffer[0], buffer_size_read);

    
    uint32_t volume_db_mono = microphone_settings.volume_db[0];
    {
      int num_of_frames_read = num_bytes_read/(4 * 2);
      for(uint32_t i = 0; i < num_of_frames_read; i++){
        if(microphone_settings.resolution == 24){
          int32_t mono_24b = (int32_t)buffer[i].left << FORMAT_24B_TO_24B_SHIFT_VAL; // Magic number
          mono_24b = i2s_to_usb_32b_sample_convert(mono_24b, volume_db_mono);

          mic_32b_i2s_buffer[i] = mono_24b; // TODO: check this value
        }
        else {
          int32_t mono_24b = (int32_t)buffer[i].left >> FORMAT_24B_TO_16B_SHIFT_VAL; // Magic number
          mono_24b = i2s_to_usb_16b_sample_convert(mono_24b, volume_db_mono);

          mic_16b_i2s_buffer[i]   = mono_24b; // TODO: check this value
        }
      }
    }
  }
}
#endif // CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX == 2

uint32_t get_num_of_mic_samples(){
  static uint32_t format_44100_khz_counter = 0;
  if(microphone_settings.sample_rate == 44100){
    format_44100_khz_counter++;
    if(format_44100_khz_counter >= 9){
      format_44100_khz_counter = 0;
      return 45;
    } else {
      return 44;
    }
  } else {
    return microphone_settings.samples_in_i2s_frame_min;
  }
}


int32_t i2s_to_usb_32b_sample_convert(int32_t sample, uint32_t volume_db){
  #ifdef APPLY_VOLUME_FEATURE
    if(microphone_settings.user_mute) 
      return 0;
    else
    {
      int64_t sample_tmp = (int64_t)sample * (int64_t)volume_db;
      sample_tmp = sample_tmp>>(15+15);
      return (int32_t)sample_tmp;
      //return (int32_t)sample;
    }
  #else //APPLY_VOLUME_FEATURE
    if(microphone_settings.user_mute) 
      return 0;
    else
    {
      return (int32_t)sample;
    }
  #endif //APPLY_VOLUME_FEATURE
}

int16_t i2s_to_usb_16b_sample_convert(int16_t sample, uint32_t volume_db){
  #ifdef APPLY_VOLUME_FEATURE
    if(microphone_settings.user_mute) 
      return 0;
    else
    {
      int64_t sample_tmp = (int64_t)(sample) * (int64_t)volume_db;
      sample_tmp = sample_tmp>>(15+15);
      return (int16_t)sample_tmp;
      //return (int16_t)sample;
    }
  #else //APPLY_VOLUME_FEATURE
    if(microphone_settings.user_mute) 
      return 0;
    else
    {
      return (int16_t)(sample);
    }
  #endif //APPLY_VOLUME_FEATURE
}


//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinking_task(__unused void *params){
  gpio_init(MIC_STATUS_LED);
  gpio_set_dir(MIC_STATUS_LED, GPIO_OUT);

  gpio_init(MIC_MUTE_LED);
  gpio_set_dir(MIC_MUTE_LED, GPIO_OUT);

  bool led_state = false;
  while(true){
    uint32_t blink_interval_ms = microphone_settings.blink_interval_ms;
    vTaskDelay(blink_interval_ms);

    //board_led_write(led_state);
    gpio_put(MIC_STATUS_LED, led_state);
    led_state = 1 - led_state;

    gpio_put(MIC_MUTE_LED, microphone_settings.user_mute);
  }
}

//--------------------------------------------------------------------+
// STATUS UPDATE TASK
//--------------------------------------------------------------------+
void status_update_task(__unused void *params){
  while(true){
    vTaskDelay(500);

    if(microphone_settings.status_updated == true){
      microphone_settings.status_updated = false;
      display_ssd1306_info();
    }
  }

}

void display_ssd1306_info(void) {
  char fmt_tmp_str[20] = "";

  ssd1306_clear(&disp);

  switch (microphone_settings.blink_interval_ms) {
    case BLINK_NOT_MOUNTED: {
      ssd1306_draw_string(&disp, 4, 0, 1, "Mic not mounted");
      break;
    }
    case BLINK_SUSPENDED: {
      ssd1306_draw_string(&disp, 4, 0, 1, "Mic suspended");
      break;
    }
    case BLINK_MOUNTED: {
      ssd1306_draw_string(&disp, 4, 0, 1, "Mic mounted");
      break;
    }
    case BLINK_STREAMING: {
      char spk_streaming_str[20] = "Mic stream: ";
      memset(fmt_tmp_str, 0x0, sizeof(fmt_tmp_str));

      itoa(microphone_settings.resolution, fmt_tmp_str, 10);
      strcat(spk_streaming_str, fmt_tmp_str);
      strcat(spk_streaming_str, " bit");

      ssd1306_draw_string(&disp, 4, 0, 1, spk_streaming_str);
      break;
    }
    default: {
      ssd1306_draw_string(&disp, 4, 0, 1, "Mic unknown");
      break;
    }
  }

  

  {
    char freq_str[20] = "Freq: ";

    memset(fmt_tmp_str, 0x0, sizeof(fmt_tmp_str));

    itoa((microphone_settings.sample_rate), fmt_tmp_str, 10);
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
    itoa((microphone_settings.volume[0]),
        fmt_tmp_str, 10);
    strcat(vol_m_str, fmt_tmp_str);

    memset(fmt_tmp_str, 0x0, sizeof(fmt_tmp_str));
    itoa((microphone_settings.volume[1]),
        fmt_tmp_str, 10);
    strcat(vol_l_str, fmt_tmp_str);

    memset(fmt_tmp_str, 0x0, sizeof(fmt_tmp_str));
    itoa((microphone_settings.volume[2]),
        fmt_tmp_str, 10);
    strcat(vol_r_str, fmt_tmp_str);

    ssd1306_draw_string(&disp, 4, 24, 1, vol_m_str);
    ssd1306_draw_string(&disp, 4, 32, 1, vol_l_str);
    ssd1306_draw_string(&disp, 4, 40, 1, vol_r_str);

    strcat(mute_m_str, (microphone_settings.mute[0] ? "T" : "F"));
    strcat(mute_l_str, (microphone_settings.mute[1] ? "T" : "F"));
    strcat(mute_r_str, (microphone_settings.mute[2] ? "T" : "F"));

    ssd1306_draw_string(&disp, 68, 24, 1, mute_m_str);
    ssd1306_draw_string(&disp, 68, 32, 1, mute_l_str);
    ssd1306_draw_string(&disp, 68, 40, 1, mute_r_str);
  }
  ssd1306_show(&disp);
}

