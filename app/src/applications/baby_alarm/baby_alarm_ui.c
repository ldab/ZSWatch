/**
 * @file baby_alarm_ui.c
 * @author Leonardo Bispo
 *
 * @brief @todo
 */

#include "baby_alarm_ui.h"

static lv_obj_t *root_page = NULL;
static lv_obj_t *counter_label;

void baby_alarm_ui_show(lv_obj_t *root)
{
    assert(root_page == NULL);

    // Create the root container
    root_page = lv_obj_create(root);
    // Remove the default border
    lv_obj_set_style_border_width(root_page, 0, LV_PART_MAIN);
    // Make root container fill the screen
    lv_obj_set_size(root_page, LV_PCT(100), LV_PCT(100));
    // Don't want it to be scollable. Putting anything close the edges
    // then LVGL automatically makes the page scrollable and shows a scroll bar.
    // Does not loog very good on the round display.
    lv_obj_set_scrollbar_mode(root_page, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(root_page, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Add the timer counter label
    counter_label = lv_label_create(root_page);
    lv_obj_align(counter_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(counter_label, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(counter_label, "-");
}

void baby_alarm_ui_remove(void)
{
    lv_obj_del(root_page);
    root_page = NULL;
}

void baby_alarm_ui_set_timer_counter_value(int value)
{
    uint8_t min = value / 60;
    uint8_t sec = value % 60;
    lv_label_set_text_fmt(counter_label, "%02d:%02d", min, sec);
}
