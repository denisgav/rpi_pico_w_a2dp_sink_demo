#pragma once

enum
{
  ITF_NUM_HID1,
  ITF_NUM_TOTAL
};

enum {
	REPORT_ID_CONSUMER_CONTROL = 1,
	REPORT_ID_COUNT
};

#define TUD_HID_MULTIMEDIA_CONSUMER_SCAN_NEXT_CODE            0x1
#define TUD_HID_MULTIMEDIA_CONSUMER_SCAN_PREVIOUS_CODE        0x2
#define TUD_HID_MULTIMEDIA_CONSUMER_STOP_CODE                 0x4
#define TUD_HID_MULTIMEDIA_CONSUMER_PLAY_PAUSE_CODE           0x8
#define TUD_HID_MULTIMEDIA_CONSUMER_VOLUME_CODE               0x10
#define TUD_HID_MULTIMEDIA_CONSUMER_MUTE_CODE                 0x20
#define TUD_HID_MULTIMEDIA_CONSUMER_VOLUME_INCREMENT_CODE     0x40
#define TUD_HID_MULTIMEDIA_CONSUMER_VOLUME_DECREMENT_CODE     0x80

// Consumer Control Report Descriptor Template
#define TUD_HID_REPORT_DESC_MULTIMEDIA_CONSUMER(...) \
		 HID_USAGE_PAGE ( HID_USAGE_PAGE_CONSUMER ),        /* Usage Page (Consumer)                                                          */\
		 HID_USAGE      ( HID_USAGE_CONSUMER_CONTROL ),     /* Usage (Consumer Control)                                                       */\
		 HID_COLLECTION ( HID_COLLECTION_APPLICATION ),     /* Collection (Application)                                                       */\
		 /* Report ID if any */                                                                                									\
		 __VA_ARGS__                                                                                           									\
		 HID_USAGE_PAGE ( HID_USAGE_PAGE_CONSUMER ),        /*   Usage Page (Consumer)                                                        */\
		 HID_LOGICAL_MIN(0x00),                             /*   Logical Minimum (0)                                                          */\
		 HID_LOGICAL_MAX(0x01),                             /*   Logical Maximum (1)                                                          */\
		 HID_REPORT_SIZE(0x01),                             /*   Report Size (1)                                                              */\
		 HID_REPORT_COUNT(0x08),                            /*   Report Count (8)                                                             */\
		 HID_USAGE(HID_USAGE_CONSUMER_SCAN_NEXT            ),/*   Usage (Scan Next Track)                                                     */\
		 HID_USAGE(HID_USAGE_CONSUMER_SCAN_PREVIOUS        ),/*   Usage (Scan Previous Track)                                                 */\
		 HID_USAGE(HID_USAGE_CONSUMER_STOP                 ),/*   Usage (Stop)                                                                */\
		 HID_USAGE(HID_USAGE_CONSUMER_PLAY_PAUSE           ),/*   Usage (Play/Pause)                                                          */\
		 HID_USAGE(HID_USAGE_CONSUMER_VOLUME               ),/*   Usage (Eject)                                                               */\
		 HID_USAGE(HID_USAGE_CONSUMER_MUTE                 ),/*   Usage (Mute)                                                                */\
		 HID_USAGE(HID_USAGE_CONSUMER_VOLUME_INCREMENT     ),/*   Usage (Volume Increment)                                                    */\
		 HID_USAGE(HID_USAGE_CONSUMER_VOLUME_DECREMENT     ),/*   Usage (Volume Decrement)                                                    */\
		 /*   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)        */                                                	\
		 HID_INPUT        ( HID_DATA | HID_VARIABLE | HID_WRAP_NO | HID_LINEAR | HID_PREFERRED_STATE | HID_NO_NULL_POSITION ),                  \
		 HID_COLLECTION_END,              /* End Collection                                                                                   */
