#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "bsp/board_api.h"
#include "tusb.h"
#include "usb_descriptors.h"

/* Blink pattern
 * - 25 ms   : streaming data
 * - 250 ms  : device not mounted
 * - 1000 ms : device mounted
 * - 2500 ms : device is suspended
 */
enum
{
  BLINK_STREAMING = 25,
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED = 1000,
  BLINK_SUSPENDED = 2500,
};

void usb_headset_init(void);
void usb_headset_task(void);


// Callback functions:
typedef void (*usb_headset_mute_set_cb_t)(int8_t bChannelNumber, int8_t mute);
typedef void (*usb_headset_volume_set_cb_t)(int8_t bChannelNumber, int16_t volume);

typedef void (*usb_headset_current_sample_rate_set_cb_t)(uint32_t current_sample_rate);
typedef void (*usb_headset_current_resolution_set_cb_t)(uint8_t itf, uint8_t current_resolution);

typedef void (*usb_headset_current_status_set_cb_t)(uint8_t itf, uint32_t blink_interval_ms);

typedef void (*usb_headset_tud_audio_rx_done_pre_read_cb_t)(uint8_t rhport, uint16_t n_bytes_received, uint8_t func_id, uint8_t ep_out, uint8_t cur_alt_setting);
typedef void (*usb_headset_tud_audio_tx_done_pre_load_cb_t)(uint8_t rhport, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting);
typedef void (*usb_headset_tud_audio_tx_done_post_load_cb_t)(uint8_t rhport, uint16_t n_bytes_copied, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting);

void usb_headset_set_mute_set_handler(usb_headset_mute_set_cb_t handler);
void usb_headset_set_volume_set_handler(usb_headset_volume_set_cb_t handler);

void usb_headset_set_current_sample_rate_set_handler(usb_headset_current_sample_rate_set_cb_t handler);
void usb_headset_set_current_resolution_set_handler(usb_headset_current_resolution_set_cb_t handler);

void usb_headset_set_current_status_set_handler(usb_headset_current_status_set_cb_t handler);

void usb_headset_set_tud_audio_rx_done_pre_read_set_handler(usb_headset_tud_audio_rx_done_pre_read_cb_t handler);
void usb_headset_set_tud_audio_tx_done_pre_load_set_handler(usb_headset_tud_audio_tx_done_pre_load_cb_t handler);
void usb_headset_set_tud_audio_tx_done_post_load_set_handler(usb_headset_tud_audio_tx_done_post_load_cb_t handler);

#ifdef __cplusplus
}
#endif
