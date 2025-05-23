#pragma once 

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "tusb_config.h"

typedef struct _usb_headset_settings_t {
	// Audio controls
	// Current states
	int8_t spk_mute[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX + 1]; // +1 for master channel 0
	int16_t spk_volume[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX + 1]; // +1 for master channel 0
	uint16_t spk_volume_db[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX + 1]; // +1 for master channel 0
	uint32_t spk_volume_mul_db[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX];

	uint32_t spk_sample_rate;
	uint8_t spk_resolution;
	uint32_t spk_blink_interval_ms;

	uint16_t spk_samples_in_i2s_frame_min;
	uint16_t spk_samples_in_i2s_frame_max;

	int8_t mic_mute[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1]; // +1 for master channel 0
	int16_t mic_volume[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1]; // +1 for master channel 0
	uint16_t mic_volume_db[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1]; // +1 for master channel 0
	uint32_t mic_volume_mul_db[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX];

	uint32_t mic_sample_rate;
	uint8_t mic_resolution;
	uint32_t mic_blink_interval_ms;
	bool usr_mic_mute;

	uint16_t mic_samples_in_i2s_frame_min;
	uint16_t mic_samples_in_i2s_frame_max;

	bool status_updated;
} usb_headset_settings_t;

#ifdef __cplusplus
}
#endif
