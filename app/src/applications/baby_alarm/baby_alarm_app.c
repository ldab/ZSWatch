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
#include <ei_wrapper.h>

#include "sample_data.h"

#define FRAME_ADD_INTERVAL_MS   100

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
    }

    if (err) {
        LOG_INF("Cannot get classification results (err: %d)", err);
    } else {
        if (ei_wrapper_classifier_has_anomaly()) {
            err = ei_wrapper_get_anomaly(&anomaly);
            if (err) {
                LOG_INF("Cannot get anomaly (err: %d)", err);
            } else {
                LOG_INF("Anomaly: %.2f", anomaly);
            }
        }
    }

    if (frame_surplus > 0) {
        err = ei_wrapper_start_prediction(0, 1);
        if (err) {
            LOG_INF("Cannot restart prediction (err: %d)", err);
        } else {
            LOG_INF("Prediction restarted...");
        }

        frame_surplus--;
    }

    // send to gadgetbridge https://www.espruino.com/Gadgetbridge
    char request[] = "{\"t\":\"info\", \"msg\":\"kid is awake\"} \n";
    ble_comm_send(request, strlen(request));

    /// @todo adv BTHome https://bthome.io/format/
}

static int baby_alarm_app_init()
{
    int err = ei_wrapper_init(result_ready_cb);

    if (err) {
        LOG_INF("Edge Impulse wrapper failed to initialize (err: %d)",
                err);
        return 0;
    };

    if (ARRAY_SIZE(input_data) < ei_wrapper_get_window_size()) {
        LOG_INF("Not enough input data");
        return 0;
    }

    if (ARRAY_SIZE(input_data) % ei_wrapper_get_frame_size() != 0) {
        LOG_INF("Improper number of input samples");
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
        LOG_INF("Cannot provide input data (err: %d)", err);
        LOG_INF("Increase CONFIG_EI_WRAPPER_DATA_BUF_SIZE");
        return 0;
    }
    cnt += ei_wrapper_get_window_size();

    err = ei_wrapper_start_prediction(0, 0);
    if (err) {
        LOG_INF("Cannot start prediction (err: %d)", err);
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
            LOG_INF("Cannot provide input data (err: %d)", err);
            LOG_INF("Increase CONFIG_EI_WRAPPER_DATA_BUF_SIZE");
            return 0;
        }
        cnt += ei_wrapper_get_frame_size();

        k_sleep(K_MSEC(FRAME_ADD_INTERVAL_MS));
    }

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
