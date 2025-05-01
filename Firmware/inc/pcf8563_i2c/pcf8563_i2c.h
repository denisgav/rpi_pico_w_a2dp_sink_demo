#pragma once

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"

#include "build_defs.h"

#define PCF8563_ADDR_DEF 0x51

typedef enum {
    // Control and status registers
    PCF8563_REG_CONTROL_STATUS_1 = 0x0,
    PCF8563_REG_CONTROL_STATUS_2 = 0x1,

    //Time and date registers
    PCF8563_REG_VL_SECONDS       = 0x2,
    PCF8563_REG_MINUTES          = 0x3,
    PCF8563_REG_HOURS            = 0x4,
    PCF8563_REG_DAYS             = 0x5,
    PCF8563_REG_WEEKDAYS         = 0x6,
    PCF8563_REG_CENTURY_MONTHS   = 0x7,
    PCF8563_REG_YEARS            = 0x8,

    //Alarm registers
    PCF8563_REG_MINUTE_ALARM     = 0x9,
    PCF8563_REG_HOUR_ALARM       = 0xA,
    PCF8563_REG_DAY_ALARM        = 0xB,
    PCF8563_REG_WEEKDAY_ALARM    = 0xC,

    //CLKOUT control register
    PCF8563_REG_CLKOUT_CONTROL   = 0xD,

    //Timer registers
    PCF8563_REG_TIMER_CONTROL    = 0xE,
    PCF8563_REG_TIMER            = 0xF
} pcf8532_registers_t;

typedef struct __pcf8532_time_and_date_t {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day_of_month;
    uint8_t day_of_week;
    uint8_t month;
    uint8_t year;
} pcf8532_time_and_date_t;

typedef union __pcf8532_time_and_date_u{
	pcf8532_time_and_date_t time_and_date;
	uint8_t time_and_date_val[7];
} pcf8532_time_and_date_u;

typedef struct __pcf8532_alarn_t {
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t day_of_week;
} pcf8532_alarm_t;

typedef union __pcf8532_alarm_u{
    pcf8532_alarm_t alarm;
    uint8_t alarm_val[4];
} pcf8532_alarm_u;

uint8_t pcf8563_seconds_to_bcd(uint8_t data);
uint8_t pcf8563_bcd_to_seconds(uint8_t data);

uint8_t pcf8563_day_of_week(uint8_t d, uint8_t m, uint16_t y);
pcf8532_time_and_date_t pcf8563_get_compilation_time();

void pcf8563_reset(i2c_inst_t *i2c);

void pcf8563_set_time_and_date(i2c_inst_t *i2c, pcf8532_time_and_date_t time);
pcf8532_time_and_date_t pcf8563_read_time_and_date(i2c_inst_t *i2c);

void pcf8563_set_alarm(i2c_inst_t *i2c, pcf8532_alarm_t alarm);
pcf8532_alarm_t pcf8563_read_alarm(i2c_inst_t *i2c);

// void pcf8563_check_alarm(i2c_inst_t *i2c);
// void pcf8563_convert_time(int conv_time[7], const uint8_t raw_time[7]);

