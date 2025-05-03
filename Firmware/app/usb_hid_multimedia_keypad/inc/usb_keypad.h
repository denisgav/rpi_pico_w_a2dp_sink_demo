#ifndef _USB_KEYPAD_H_
#define _USB_KEYPAD_H_

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

void usb_keypad_init();
void usb_keypad_task();

typedef void     (*usb_keypad_current_status_set_cb_t)(uint32_t blink_interval_ms);
typedef uint16_t (*usb_keypad_get_report_cb_t)(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen);
typedef void     (*usb_keypad_set_report_cb_t)(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize);

void usb_keypad_set_current_status_set_handler(usb_keypad_current_status_set_cb_t handler);
void usb_keypad_set_get_report_handler(usb_keypad_get_report_cb_t handler);
void usb_keypad_set_set_report_handler(usb_keypad_set_report_cb_t handler);

#endif