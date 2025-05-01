/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"

#include "pcf8563_i2c/pcf8563_i2c.h"

/* Example code to talk to a PCF8520 Real Time Clock module 

   Connections on Raspberry Pi Pico board, other boards may vary.

   GPIO PICO_DEFAULT_I2C_SDA_PIN (On Pico this is 4 (physical pin 6)) -> SDA on PCF8520 board
   GPIO PICO_DEFAULT_I2C_SCK_PIN (On Pico this is 5 (physical pin 7)) -> SCL on PCF8520 board
   5V (physical pin 40) -> VCC on PCF8520 board
   GND (physical pin 38)  -> GND on PCF8520 board
*/

// By default these devices  are on bus address 0x68
const int PCF8563_ADDR = PCF8563_ADDR_DEF;

uint8_t pcf8563_seconds_to_bcd(uint8_t data){
    uint8_t upper_digit = data / 10;
    uint8_t digit = data % 10;

    uint8_t res = (upper_digit << 4) + digit;

    return res;
}

uint8_t pcf8563_bcd_to_seconds(uint8_t data){
    uint8_t upper_digit = data >> 4;
    uint8_t digit = data & 0xF;

    uint8_t res = upper_digit*10 + digit;

    return res;
}

// Function to calculate the day of the week using the formula-based approach
uint8_t pcf8563_day_of_week(uint8_t d, uint8_t m, uint16_t y) {
    // Predefined month codes for each month
    static uint16_t month_code[] = {6, 2, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};

    // Adjust year for January and February
    if (m < 3) {
        y -= 1;  // If month is January or February, treat them as part of the previous year
    }

    // Calculate the year code
    uint16_t year_code = (y % 100) + (y % 100) / 4;

    // Adjust year code for the century
    year_code = (year_code + (y / 100) / 4 + 5 * (y / 100)) % 7;

    // Calculate the day of the week and return the value as an integer
    return (d + month_code[m - 1] + year_code) % 7;
}

pcf8532_time_and_date_t pcf8563_get_compilation_time(){
    pcf8532_time_and_date_t res;

    res.second          = BUILD_SEC;
    res.minute          = BUILD_MIN;
    res.hour            = BUILD_HOUR;

    res.day_of_month    = BUILD_DAY;
    res.month           = BUILD_MONTH;
    res.year            = BUILD_YEAR - 2000; // TODO: check patch with 2000

    res.day_of_week     = pcf8563_day_of_week(res.day_of_month, res.month, res.year);

    return res;
}

void pcf8563_reset(i2c_inst_t *i2c) {
    // Two byte reset. First byte register, second byte data
    // There are a load more options to set up the device in different ways that could be added here
    uint8_t buf[] = {PCF8563_REG_CONTROL_STATUS_1, 0x2};
    i2c_write_blocking(i2c, PCF8563_ADDR, buf, 2, false);
}

void pcf8563_set_time_and_date(i2c_inst_t *i2c, pcf8532_time_and_date_t time_and_date) {
    // buf[0] is the register to write to
    // buf[1] is the value that will be written to the register
    uint8_t buf[2];

    time_and_date.second = pcf8563_seconds_to_bcd(time_and_date.second);

    pcf8532_time_and_date_u cur_time_and_date_u;
    cur_time_and_date_u.time_and_date = time_and_date;

    for(int i=0; i<sizeof(pcf8532_time_and_date_t); i++){
       buf[0] = PCF8563_REG_VL_SECONDS + i; 
       buf[1] = cur_time_and_date_u.time_and_date_val[i]; 

       i2c_write_blocking(i2c, PCF8563_ADDR, buf, 2, false);
    }
}

pcf8532_time_and_date_t pcf8563_read_time_and_date(i2c_inst_t *i2c) {
    // For this particular device, we send the device the register we want to read
    // first, then subsequently read from the device. The register is auto incrementing
    // so we don't need to keep sending the register we want, just the first.

    pcf8532_time_and_date_u cur_time_and_date_u;

    // Start reading acceleration registers from register 0x3B for 6 bytes
    uint8_t val = PCF8563_REG_VL_SECONDS;
    i2c_write_blocking(i2c, PCF8563_ADDR, &val, 1, true); // true to keep master control of bus
    i2c_read_blocking(i2c, PCF8563_ADDR, cur_time_and_date_u.time_and_date_val, sizeof(cur_time_and_date_u.time_and_date_val), false);

    cur_time_and_date_u.time_and_date.second = pcf8563_bcd_to_seconds(cur_time_and_date_u.time_and_date.second & 0x7F);

    return cur_time_and_date_u.time_and_date;
}


void pcf8563_set_alarm(i2c_inst_t *i2c, pcf8532_alarm_t alarm) {
    // buf[0] is the register to write to
    // buf[1] is the value that will be written to the register
    uint8_t buf[2];

    pcf8532_alarm_u alarm_u;
    alarm_u.alarm = alarm;

    // Write alarm values to registers
    for(int i=0; i<sizeof(pcf8532_alarm_t); i++){
       buf[0] = PCF8563_REG_MINUTE_ALARM + i; 
       buf[1] = alarm_u.alarm_val[i]; 

       i2c_write_blocking(i2c, PCF8563_ADDR, buf, 2, false);
    }
}

pcf8532_alarm_t pcf8563_read_alarm(i2c_inst_t *i2c){
    pcf8532_alarm_u alarm_u;

    // Start reading acceleration registers from register 0x3B for 6 bytes
    uint8_t val = PCF8563_REG_MINUTE_ALARM;
    i2c_write_blocking(i2c, PCF8563_ADDR, &val, 1, true); // true to keep master control of bus
    i2c_read_blocking(i2c, PCF8563_ADDR, alarm_u.alarm_val, sizeof(alarm_u.alarm_val), false);

    return alarm_u.alarm;
}

// void pcf8563_check_alarm(i2c_inst_t *i2c) {
//     // Check bit 3 of control register 2 for alarm flags
//     uint8_t status[1];
//     uint8_t val = 0x01;
//     i2c_write_blocking(i2c, PCF8563_ADDR, &val, 1, true); // true to keep master control of bus
//     i2c_read_blocking(i2c, PCF8563_ADDR, status, 1, false);

//     if ((status[0] & 0x08) == 0x08) {
//         printf("ALARM RINGING");
//     } else {
//         printf("Alarm not triggered yet");
//     }
// }


// void pcf8563_convert_time(int conv_time[7], const uint8_t raw_time[7]) {
//     // Convert raw data into time
//     conv_time[0] = (10 * (int) ((raw_time[0] & 0x70) >> 4)) + ((int) (raw_time[0] & 0x0F));
//     conv_time[1] = (10 * (int) ((raw_time[1] & 0x70) >> 4)) + ((int) (raw_time[1] & 0x0F));
//     conv_time[2] = (10 * (int) ((raw_time[2] & 0x30) >> 4)) + ((int) (raw_time[2] & 0x0F));
//     conv_time[3] = (10 * (int) ((raw_time[3] & 0x30) >> 4)) + ((int) (raw_time[3] & 0x0F));
//     conv_time[4] = (int) (raw_time[4] & 0x07);
//     conv_time[5] = (10 * (int) ((raw_time[5] & 0x10) >> 4)) + ((int) (raw_time[5] & 0x0F));
//     conv_time[6] = (10 * (int) ((raw_time[6] & 0xF0) >> 4)) + ((int) (raw_time[6] & 0x0F));
// }

