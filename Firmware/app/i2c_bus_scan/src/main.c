/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Sweep through all 7-bit I2C addresses, to see if any slaves are present on
// the I2C bus. Print out a table that looks like this:
//
// I2C Bus Scan
//    0 1 2 3 4 5 6 7 8 9 A B C D E F
// 00 . . . . . . . . . . . . . . . .
// 10 . . @ . . . . . . . . . . . . .
// 20 . . . . . . . . . . . . . . . .
// 30 . . . . @ . . . . . . . . . . .
// 40 . . . . . . . . . . . . . . . .
// 50 . . . . . . . . . . . . . . . .
// 60 . . . . . . . . . . . . . . . .
// 70 . . . . . . . . . . . . . . . .
// E.g. if addresses 0x12 and 0x34 were acknowledged.

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"

#include "board_defines.h"

// I2C reserves some addresses for special purposes. We exclude these from the scan.
// These are any addresses of the form 000 0xxx or 111 1xxx
bool reserved_addr(uint8_t addr) {
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

int main() {
    // Enable UART so we can print status output
    stdio_init_all();

    // This example will use I2C0 on the default SDA and SCL pins (GP4, GP5 on a Pico)
    i2c_init(I2C_INST, 400000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    bi_decl(bi_2pins_with_func(I2C_SDA, I2C_SCL, GPIO_FUNC_I2C));

    while(true) {
        printf("\nI2C Bus Scan\n");
        printf("   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

        for (int addr = 0; addr < (1 << 7); ++addr) {
            if (addr % 16 == 0) {
                printf("%02x ", addr);
            }

            // Perform a 1-byte dummy read from the probe address. If a slave
            // acknowledges this address, the function returns the number of bytes
            // transferred. If the address byte is ignored, the function returns
            // -1.

            // Skip over any reserved addresses.
            int ret;
            uint8_t rxdata;
            if (reserved_addr(addr))
                ret = PICO_ERROR_GENERIC;
            else
                ret = i2c_read_blocking(I2C_INST, addr, &rxdata, 1, false);

            printf(ret < 0 ? "." : "@");
            printf(addr % 16 == 15 ? "\n" : "  ");
        }
        printf("Done.\n");

        sleep_ms(1000);
    }
    return 0;
}