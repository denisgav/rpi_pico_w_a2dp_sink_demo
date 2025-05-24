#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "bsp/board_api.h"
#include "tusb.h"

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

typedef void     (*usb_current_status_set_cb_t)(uint8_t itf, uint32_t blink_interval_ms);

void usb_set_current_status_set_handler(usb_current_status_set_cb_t handler);

usb_current_status_set_cb_t usb_get_current_status_set_handler(void);

#ifdef __cplusplus
}
#endif
