#ifndef _USB_KEYPAD_H_
#define _USB_KEYPAD_H_

void usb_keypad_init();
void usb_keypad_task();

typedef uint16_t (*usb_keypad_get_report_cb_t)(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen);
typedef void     (*usb_keypad_set_report_cb_t)(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize);

void usb_keypad_set_get_report_handler(usb_keypad_get_report_cb_t handler);
void usb_keypad_set_set_report_handler(usb_keypad_set_report_cb_t handler);

#endif