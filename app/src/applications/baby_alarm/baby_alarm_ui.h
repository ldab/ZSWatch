#pragma once

/**
 * @file baby_alarm_ui.h
 * @author Leonardo Bispo
 *
 * @brief @todo
 */

#include <inttypes.h>
#include <lvgl.h>

void baby_alarm_ui_show(lv_obj_t *root);

void baby_alarm_ui_remove(void);

void baby_alarm_ui_set_timer_counter_value(int value);
