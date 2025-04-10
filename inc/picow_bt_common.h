/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

void hal_led_toggle(void);

/*
 * \brief Initialise BTstack example with cyw43
 *
 * \return 0 if ok
 */
int picow_bt_init(void);

/*
 * \brief Run the BTstack example
 *
 */
void picow_bt_main(void);
