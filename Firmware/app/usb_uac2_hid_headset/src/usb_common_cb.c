#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <pico/stdlib.h>

#include "bsp/board_api.h"
#include "tusb.h"

#include "usb_common_cb.h"

static usb_current_status_set_cb_t  usb_status_set_handler = NULL;

void usb_set_current_status_set_handler(usb_current_status_set_cb_t handler){
	usb_status_set_handler = handler;
}

usb_current_status_set_cb_t usb_get_current_status_set_handler(void){
    return usb_status_set_handler;
}


//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
	if(usb_status_set_handler != NULL)
		usb_status_set_handler(0, BLINK_MOUNTED);
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
	if(usb_status_set_handler != NULL)
		usb_status_set_handler(0, BLINK_NOT_MOUNTED);
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  if(usb_status_set_handler != NULL)
		usb_status_set_handler(0, BLINK_SUSPENDED);
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
	if(usb_status_set_handler != NULL)
		usb_status_set_handler(0, tud_mounted() ? BLINK_MOUNTED : BLINK_NOT_MOUNTED);
}