#pragma once

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"


#define PCF8563_ADDR_DEF 0x51

//Write values for the current time in the array
//index 0 -> second: bits 4-6 are responsible for the ten's digit and bits 0-3 for the unit's digit
//index 1 -> minute: bits 4-6 are responsible for the ten's digit and bits 0-3 for the unit's digit
//index 2 -> hour: bits 4-5 are responsible for the ten's digit and bits 0-3 for the unit's digit
//index 3 -> day of the month: bits 4-5 are responsible for the ten's digit and bits 0-3 for the unit's digit
//index 4 -> day of the week: where Sunday = 0x00, Monday = 0x01, Tuesday... ...Saturday = 0x06
//index 5 -> month: bit 4 is responsible for the ten's digit and bits 0-3 for the unit's digit
//index 6 -> year: bits 4-7 are responsible for the ten's digit and bits 0-3 for the unit's digit

//NOTE: if the value in the year register is a multiple for 4, it will be considered a leap year and hence will include the 29th of February

typedef struct __pcf8532_time_t {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day_of_month;
    uint8_t day_of_week;
    uint8_t month;
    uint8_t year;
} pcf8532_time_t;

typedef union __pcf8532_time_u{
	pcf8532_time_t time;
	uint8_t time_val[7];
} pcf8532_time_u;


void pcf8563_reset(i2c_inst_t *i2c);
void pcf8563_write_current_time(i2c_inst_t *i2c);
void pcf8563_read_raw(i2c_inst_t *i2c, uint8_t *buffer);
void pcf8563_set_alarm(i2c_inst_t *i2c);
void pcf8563_check_alarm(i2c_inst_t *i2c);
void pcf8563_convert_time(int conv_time[7], const uint8_t raw_time[7]);