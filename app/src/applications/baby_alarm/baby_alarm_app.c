/**
 * @file baby_alarm_app.c
 * @author Leonardo Bispo
 *
 * @brief @todo
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#include "baby_alarm_app.h"
#include "baby_alarm_ui.h"
#include "managers/zsw_app_manager.h"
#include "ble/ble_hid.h"
#include "ui/utils/zsw_ui_utils.h"

LOG_MODULE_REGISTER(baby_alarm_app, CONFIG_ZSW_BABY_ALARM_APP_LOG_LEVEL);

static void baby_alarm_app_start(lv_obj_t *root, lv_group_t *group);
static void baby_alarm_app_stop(void);
static void timer_callback(lv_timer_t *timer);

ZSW_LV_IMG_DECLARE(remote_control);

static application_t app = {
    .name = "baby_alarm",
    .icon = ZSW_LV_IMG_USE(remote_control),
    .start_func = baby_alarm_app_start,
    .stop_func = baby_alarm_app_stop
};

static lv_timer_t *counter_timer = NULL;
static int64_t start_time;

static void baby_alarm_app_start(lv_obj_t *root, lv_group_t *group)
{
    baby_alarm_ui_show(root);

    counter_timer = lv_timer_create(timer_callback, 1000, NULL);
    start_time = k_uptime_get();
}

static void baby_alarm_app_stop(void)
{
    if (counter_timer != NULL) {
        lv_timer_del(counter_timer);
    }

    baby_alarm_ui_remove();
}

static void timer_callback(lv_timer_t *timer)
{
    int64_t now = k_uptime_get();
    baby_alarm_ui_set_timer_counter_value((int)((now - start_time) / 1000));
}

static int baby_alarm_app_add(void)
{
    zsw_app_manager_add_application(&app);

    return 0;
}

SYS_INIT(baby_alarm_app_add, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
