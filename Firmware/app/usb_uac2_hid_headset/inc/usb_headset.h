#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "bsp/board_api.h"
#include "tusb.h"
#include "usb_descriptors.h"

#include "usb_common_cb.h"

void usb_headset_init(void);
void usb_headset_task(void);


// Callback functions:
typedef void (*usb_headset_mute_set_cb_t)(uint8_t itf, int8_t bChannelNumber, int8_t mute);
typedef void (*usb_headset_volume_set_cb_t)(uint8_t itf, int8_t bChannelNumber, int16_t volume);

typedef void (*usb_headset_current_sample_rate_set_cb_t)(uint8_t itf, uint32_t current_sample_rate);
typedef void (*usb_headset_current_resolution_set_cb_t)(uint8_t itf, uint8_t current_resolution);

typedef void (*usb_headset_tud_audio_rx_done_pre_read_cb_t)(uint8_t rhport, uint16_t n_bytes_received, uint8_t func_id, uint8_t ep_out, uint8_t cur_alt_setting);
typedef void (*usb_headset_tud_audio_tx_done_pre_load_cb_t)(uint8_t rhport, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting);
typedef void (*usb_headset_tud_audio_tx_done_post_load_cb_t)(uint8_t rhport, uint16_t n_bytes_copied, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting);

void usb_headset_set_mute_set_handler(usb_headset_mute_set_cb_t handler);
void usb_headset_set_volume_set_handler(usb_headset_volume_set_cb_t handler);

void usb_headset_set_current_sample_rate_set_handler(usb_headset_current_sample_rate_set_cb_t handler);
void usb_headset_set_current_resolution_set_handler(usb_headset_current_resolution_set_cb_t handler);

void usb_headset_set_tud_audio_rx_done_pre_read_set_handler(usb_headset_tud_audio_rx_done_pre_read_cb_t handler);
void usb_headset_set_tud_audio_tx_done_pre_load_set_handler(usb_headset_tud_audio_tx_done_pre_load_cb_t handler);
void usb_headset_set_tud_audio_tx_done_post_load_set_handler(usb_headset_tud_audio_tx_done_post_load_cb_t handler);

#ifdef __cplusplus
}
#endif
