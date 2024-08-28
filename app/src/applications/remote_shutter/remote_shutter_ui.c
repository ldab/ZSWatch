#include "remote_shutter_ui.h"
#include <zephyr/kernel.h>
#include <stdlib.h>

static void click_event_cb(lv_event_t *e);

static lv_obj_t *root_page = NULL;
static lv_obj_t *timer_label;
static on_button_press_cb_t click_cb;

static void slider_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);

    /*Refresh the text*/
    lv_label_set_text_fmt(timer_label, "%"LV_PRId32, lv_slider_get_value(slider));
    lv_obj_align_to(timer_label, slider, LV_ALIGN_OUT_TOP_MID, 0, -15);    /*Align top of the slider*/
}

void remote_shutter_ui_show(lv_obj_t *root, on_button_press_cb_t cb)
{
    assert(root_page == NULL);
    click_cb = cb;

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
    lv_obj_set_style_bg_opa(root_page, LV_OPA_TRANSP,
                            LV_PART_MAIN | LV_STATE_DEFAULT);

    // Create a slider in the center of the display
    lv_obj_t *slider = lv_slider_create(root_page);
    lv_obj_set_width(slider, LV_PCT(80));
    lv_obj_align(slider, LV_ALIGN_BOTTOM_MID, 0, -25);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // Add the timer label
    timer_label = lv_label_create(root_page);
    lv_label_set_text(timer_label, "0");
    lv_obj_align_to(timer_label, slider, LV_ALIGN_OUT_TOP_MID, 0, -15);

    /*Properties to transition*/
    static lv_style_prop_t props[] = {
        LV_STYLE_TRANSFORM_WIDTH,
        LV_STYLE_TRANSFORM_HEIGHT,
        LV_STYLE_TEXT_LETTER_SPACE, 0
    };

    /*Transition descriptor when going back to the default state.
     *Add some delay to be sure the press transition is visible even if the press
     *was very short*/
    static lv_style_transition_dsc_t transition_dsc_def;
    lv_style_transition_dsc_init(&transition_dsc_def, props,
                                 lv_anim_path_overshoot, 200, 0, NULL);

    /*Transition descriptor when going to pressed state.
     *No delay, go to presses state immediately*/
    static lv_style_transition_dsc_t transition_dsc_pr;
    lv_style_transition_dsc_init(&transition_dsc_pr, props,
                                 lv_anim_path_ease_in_out, 200, 0, NULL);

    /*Add only the new transition to he default state*/
    static lv_style_t style_def;
    lv_style_init(&style_def);
    lv_style_set_radius(&style_def, 80);
    lv_style_set_size(&style_def, 80);
    lv_style_set_transition(&style_def, &transition_dsc_def);

    /*Add the transition and some transformation to the presses state.*/
    static lv_style_t style_pr;
    lv_style_init(&style_pr);
    lv_style_set_transform_width(&style_pr, 10);
    lv_style_set_transform_height(&style_pr, -10);
    lv_style_set_text_letter_space(&style_pr, 10);
    lv_style_set_transition(&style_pr, &transition_dsc_pr);

    /*Add a button the current screen*/
    lv_obj_t *shoot = lv_btn_create(root_page);
    lv_obj_align(shoot, LV_ALIGN_TOP_MID, 0, 15);
    lv_obj_add_style(shoot, &style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(shoot, &style_def, 0);

    /*Assign a callback to the button*/
    lv_obj_add_event_cb(shoot, click_event_cb, LV_EVENT_CLICKED, NULL);

    /*Add a label to the button*/
    lv_obj_t *label_shoot = lv_label_create(shoot);
    lv_label_set_text(label_shoot, "Shoot");

    lv_obj_center(label_shoot);
}

void remote_shutter_ui_remove(void)
{
    lv_obj_del(root_page);
    root_page = NULL;
}

static void click_event_cb(lv_event_t *e)
{
    uint32_t wait = atoi(lv_label_get_text(timer_label));
    k_sleep(K_SECONDS(wait));
    click_cb();
}
