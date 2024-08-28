#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "ble/ble_hid.h"
#include "managers/zsw_app_manager.h"
#include "remote_shutter_ui.h"
#include "ui/utils/zsw_ui_utils.h"

LOG_MODULE_REGISTER(remote_shutter_app, CONFIG_ZSW_REMOTE_SHUTTER_APP_LOG_LEVEL);

// Functions needed for all applications
static void remote_shutter_app_start(lv_obj_t *root, lv_group_t *group);
static void remote_shutter_app_stop(void);
static void on_shoot(void);

ZSW_LV_IMG_DECLARE(remote_control);

static application_t app = {
    .name = "Remote Shutter",
    .icon = ZSW_LV_IMG_USE(remote_control),
    .start_func = remote_shutter_app_start,
    .stop_func = remote_shutter_app_stop,
};

static void remote_shutter_app_start(lv_obj_t *root, lv_group_t *group)
{
    remote_shutter_ui_show(root, on_shoot);
}

static void remote_shutter_app_stop(void)
{
    remote_shutter_ui_remove();
}

static int remote_shutter_app_add(void)
{
    zsw_app_manager_add_application(&app);

    return 0;
}

static void on_shoot(void)
{
    ble_hid_volume_up();
}

SYS_INIT(remote_shutter_app_add, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
