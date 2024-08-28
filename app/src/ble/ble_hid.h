#pragma once

#include <zephyr/kernel.h>

int ble_hid_init(void);

int ble_hid_next(void);

int ble_hid_previous(void);

int ble_hid_volume_up(void);
