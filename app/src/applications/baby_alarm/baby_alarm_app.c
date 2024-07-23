/**
 * @file baby_alarm_app.c
 * @author Leonardo Bispo
 *
 * @brief @todo
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include "zephyr/bluetooth/uuid.h"
#include "zephyr/bluetooth/bluetooth.h"

#include "baby_alarm_app.h"
#include "baby_alarm_ui.h"

#include "managers/zsw_app_manager.h"
#include "ui/utils/zsw_ui_utils.h"

#include <ei_wrapper.h>

#include "sample_data.h"

#define FRAME_ADD_INTERVAL_MS   100
#define BTHOME_UUID 0xFCD2
#define TRIGGER_LABEL "awake"
#define VALUE_THRESHOLD 0.8

LOG_MODULE_REGISTER(baby_alarm_app, CONFIG_ZSW_BABY_ALARM_APP_LOG_LEVEL);

static void baby_alarm_app_start(lv_obj_t *root, lv_group_t *group);
static void baby_alarm_app_stop(void);
static void timer_callback(lv_timer_t *timer);

ZSW_LV_IMG_DECLARE(remote_control);

static application_t app = {
    .name = "Baby alarm",
    .icon = ZSW_LV_IMG_USE(remote_control),
    .start_func = baby_alarm_app_start,
    .stop_func = baby_alarm_app_stop
};

static lv_timer_t *counter_timer = NULL;
static int64_t start_time;
static size_t frame_surplus;

static void send_result_bt()
{
    // send to gadgetbridge https://www.espruino.com/Gadgetbridge
    char request[] = "{\"t\":\"info\", \"msg\":\"kid is awake\"} \n";
    int ret = ble_comm_send(request, strlen(request));
    if (ret) {
        LOG_ERR("ble_comm_send() failed: %d", ret);
    }

    // advertise as BTHome https://bthome.io/format/

    uint8_t bthome_ad[] = {
        BT_UUID_16_ENCODE(BTHOME_UUID),
        0x44, // BTHome v2, no encryption, irregular interval
        0x22, // moving property
        0x01, // moving
    };

    struct bt_data ad[] = {
        BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE,
                      (CONFIG_BT_DEVICE_APPEARANCE >> 0) & 0xff,
                      (CONFIG_BT_DEVICE_APPEARANCE >> 8) & 0xff),
        BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
        BT_DATA_BYTES(BT_DATA_UUID16_ALL,
                      BT_UUID_16_ENCODE(BT_UUID_DIS_VAL)),
        BT_DATA(BT_DATA_SVC_DATA16, bthome_ad, ARRAY_SIZE(bthome_ad))
    };

    ret = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);
    if (ret) {
        LOG_ERR("bt_le_adv_update_data() failed: %d", ret);
    }

    /// @todo kwork delay adv to normal
}

static void result_ready_cb(int err)
{
    if (err) {
        LOG_ERR("Result ready callback returned error (err: %d)", err);
        return;
    }

    const char *label;
    float value;
    float anomaly;

    LOG_INF("Classification results");
    LOG_INF("======================");

    while (true) {
        err = ei_wrapper_get_next_classification_result(&label, &value, NULL);

        if (err) {
            if (err == -ENOENT) {
                err = 0;
            }
            break;
        }

        LOG_INF("Value: %.2f\tLabel: %s", value, label);

        if ((strcmp(label, TRIGGER_LABEL) == 0) && (value > VALUE_THRESHOLD)) {
            send_result_bt();
        }
    }

    if (err) {
        LOG_ERR("Cannot get classification results (err: %d)", err);
    } else {
        if (ei_wrapper_classifier_has_anomaly()) {
            err = ei_wrapper_get_anomaly(&anomaly);
            if (err) {
                LOG_ERR("Cannot get anomaly (err: %d)", err);
            } else {
                LOG_INF("Anomaly: %.2f", anomaly);
            }
        }
    }

    if (frame_surplus > 0) {
        err = ei_wrapper_start_prediction(0, 1);
        if (err) {
            LOG_ERR("Cannot restart prediction (err: %d)", err);
        } else {
            LOG_INF("Prediction restarted...");
        }

        frame_surplus--;
    }

    bool cancelled;
    err = ei_wrapper_clear_data(&cancelled);
    if (err) {
        LOG_ERR("Cannot clear data (err: %d)", err);
    }
}

static int baby_alarm_app_init()
{
    int err = ei_wrapper_init(result_ready_cb);

    if ((err != -EALREADY) && (err != 0)) {
        LOG_ERR("Edge Impulse wrapper failed to initialize (err: %d)",
                err);
        return 0;
    };

    if (ARRAY_SIZE(input_data) < ei_wrapper_get_window_size()) {
        LOG_ERR("Not enough input data");
        return 0;
    }

    if (ARRAY_SIZE(input_data) % ei_wrapper_get_frame_size() != 0) {
        LOG_ERR("Improper number of input samples");
        return 0;
    }

    LOG_INF("Machine learning model sampling frequency: %zu",
            ei_wrapper_get_classifier_frequency());

    LOG_INF("Labels assigned by the model:");
    for (size_t i = 0; i < ei_wrapper_get_classifier_label_count(); i++) {
        LOG_INF("- %s", ei_wrapper_get_classifier_label(i));
    }

    size_t cnt = 0;

    /* input_data is defined in input_data.h file. */
    err = ei_wrapper_add_data(&input_data[cnt],
                              ei_wrapper_get_window_size());
    if (err) {
        LOG_ERR("Cannot provide input data (err: %d)", err);
        LOG_ERR("Increase CONFIG_EI_WRAPPER_DATA_BUF_SIZE");
        return 0;
    }
    cnt += ei_wrapper_get_window_size();

    err = ei_wrapper_start_prediction(0, 0);
    if (err) {
        LOG_ERR("Cannot start prediction (err: %d)", err);
    } else {
        LOG_INF("Prediction started...");
    }

    /* Predictions for additional data are triggered in the result ready
     * callback. The prediction start can be triggered before the input
     * data is provided. In that case the prediction is started right
     * after the prediction window is filled with data.
     */
    frame_surplus = (ARRAY_SIZE(input_data) - ei_wrapper_get_window_size())
                    / ei_wrapper_get_frame_size();

    while (cnt < ARRAY_SIZE(input_data)) {
        err = ei_wrapper_add_data(&input_data[cnt],
                                  ei_wrapper_get_frame_size());
        if (err) {
            LOG_ERR("Cannot provide input data (err: %d)", err);
            LOG_ERR("Increase CONFIG_EI_WRAPPER_DATA_BUF_SIZE");
            return 0;
        }
        cnt += ei_wrapper_get_frame_size();

        k_sleep(K_MSEC(FRAME_ADD_INTERVAL_MS));
    }

    cnt = 0;
    return 0;
}

static void baby_alarm_app_start(lv_obj_t *root, lv_group_t *group)
{
    baby_alarm_app_init();
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
