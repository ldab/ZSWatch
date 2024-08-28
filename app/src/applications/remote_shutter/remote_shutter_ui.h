#pragma once

#include <inttypes.h>
#include <lvgl.h>

typedef void (*on_button_press_cb_t)(void);

void remote_shutter_ui_show(lv_obj_t *root, on_button_press_cb_t cb);

void remote_shutter_ui_remove(void);
