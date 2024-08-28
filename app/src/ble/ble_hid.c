/**
 * @file ble_hid.c
 * @author Leonardo Bispo
 *
 * @brief Implements Human Interface Device (HID) over BLE GATT Profile.
 *
 * @see https://www.bluetooth.com/specifications/specs/hid-over-gatt-profile-1-0/
 */

#include <bluetooth/services/hids.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(ble_hid, CONFIG_ZSW_BLE_LOG_LEVEL);

#define BASE_USB_HID_SPEC_VERSION   0x0101

#define INPUT_REP_IDX           0x00
#define REPORT_ID_CONSUMER_CTRL 0x00
#define REPORT_ID_KEY_CTRL      0x01
#define REPORT_SIZE             0x08 // bytes

#define HID_KEY_RIGHT_ARROW     0x4F
#define HID_KEY_LEFT_ARROW      0x50
#define HID_USAGE_VOLUME_UP     0xE9

/* HIDS instance. */
BT_HIDS_DEF(hids_obj, REPORT_SIZE);

static void key_release_handle(struct k_work *item);
K_WORK_DELAYABLE_DEFINE(key_release, key_release_handle);

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (!err) {
        int error = bt_hids_connected(&hids_obj, conn);

        if (error) {
            LOG_ERR("Failed to notify HIDS about connection %d", error);
        }
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    int err = bt_hids_disconnected(&hids_obj, conn);

    if (err) {
        LOG_ERR("Failed to notify HIDS about disconnection %d", err);
    }
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected        = connected,
    .disconnected     = disconnected,
};

static void key_release_handle(struct k_work *item)
{
    int8_t buf[REPORT_SIZE];
    memset(buf, 0, sizeof(buf));
    // bt_gatt_notify(NULL, &hog_svc.attrs[5], report, sizeof(report));
    bt_hids_inp_rep_send(&hids_obj, NULL, INPUT_REP_IDX, buf, ARRAY_SIZE(buf), NULL);
}

static void send_key_release(void)
{
    k_work_reschedule(&key_release, K_MSEC(250));
}

int ble_hid_init(void)
{
    int err;
    struct bt_hids_init_param hids_init_param = { 0 };
    struct bt_hids_inp_rep *rep;

    static const uint8_t report_map[] = {
#ifdef CONFIG_APPLICATIONS_USE_REMOTE_SHUTTER
        0x05, 0x0C,             /* Usage page (Consumer Control) */
        0x09, 0x01,             /* Usage (Consumer Control) */
        0xA1, 0x01,             /* Collection (Application) */
        0x85, REPORT_ID_CONSUMER_CTRL,      /* Report ID */
        0x15, 0x00,             /* Logical minimum (0) */
        0x26, 0xFF, 0x03,           /* Logical maximum (0x3FF) */
        0x19, 0x00,             /* Usage minimum (0) */
        0x2A, 0xFF, 0x03,           /* Usage maximum (0x3FF) */
        0x75, 0x10,             /* Report Size (16) */
        0x95, 0x01,             /* Report Count */
        0x81, 0x00,             /* Input (Data,Array,Absolute) */
        0xC0,                   /* End Collection (Application) */
#endif
#ifdef CONFIG_APPLICATIONS_USE_PPT_REMOTE
        0x05, 0x01,       /* Usage Page (Generic Desktop) */
        0x09, 0x06,       /* Usage (Keyboard) */
        0xA1, 0x01,       /* Collection (Application) */

        /* Keys */
        0x05, 0x07,       /* Usage Page (Key Codes) */
        0x19, 0xe0,       /* Usage Minimum (224) */
        0x29, 0xe7,       /* Usage Maximum (231) */
        0x15, 0x00,       /* Logical Minimum (0) */
        0x25, 0x01,       /* Logical Maximum (1) */
        0x75, 0x01,       /* Report Size (1) */
        0x95, 0x08,       /* Report Count (8) */
        0x81, 0x02,       /* Input (Data, Variable, Absolute) */

        0x95, 0x01,       /* Report Count (1) */
        0x75, 0x08,       /* Report Size (8) */
        0x81, 0x01,       /* Input (Constant) reserved byte(1) */

        0x95, 0x06,       /* Report Count (6) */
        0x75, 0x08,       /* Report Size (8) */
        0x15, 0x00,       /* Logical Minimum (0) */
        0x25, 0x65,       /* Logical Maximum (101) */
        0x05, 0x07,       /* Usage Page (Key codes) */
        0x19, 0x00,       /* Usage Minimum (0) */
        0x29, 0x65,       /* Usage Maximum (101) */
        0x81, 0x00,       /* Input (Data, Array) Key array(6 bytes) */

        /* LED */
        0x95, 0x05,       /* Report Count (5) */
        0x75, 0x01,       /* Report Size (1) */
        0x05, 0x08,       /* Usage Page (Page# for LEDs) */
        0x19, 0x01,       /* Usage Minimum (1) */
        0x29, 0x05,       /* Usage Maximum (5) */
        0x91, 0x02,       /* Output (Data, Variable, Absolute), */
        /* Led report */
        0x95, 0x01,       /* Report Count (1) */
        0x75, 0x03,       /* Report Size (3) */
        0x91, 0x01,       /* Output (Data, Variable, Absolute), */
        /* Led report padding */

        0xC0              /* End Collection (Application) */
#endif
    };

    hids_init_param.rep_map.data = report_map;
    hids_init_param.rep_map.size = sizeof(report_map);

    hids_init_param.info.bcd_hid = BASE_USB_HID_SPEC_VERSION;
    hids_init_param.info.b_country_code = 0x00;
    hids_init_param.info.flags = (BT_HIDS_REMOTE_WAKE | BT_HIDS_NORMALLY_CONNECTABLE);

    rep = &hids_init_param.inp_rep_group_init.reports[INPUT_REP_IDX];
    rep->id = REPORT_ID_CONSUMER_CTRL;
    rep->size = REPORT_SIZE;

    hids_init_param.inp_rep_group_init.cnt = 1;

    err = bt_hids_init(&hids_obj, &hids_init_param);

    return err;
}

int ble_hid_next(void)
{
    int ret = 0;

    // Byte 0: Modifier
    // Byte 1: Reserved
    // Byte 2: Keypress
    int8_t buf[REPORT_SIZE] = {0x00, 0x00, HID_KEY_RIGHT_ARROW};
    // ret = bt_gatt_notify(NULL, &hog_svc.attrs[5], report, sizeof(report));
    ret = bt_hids_inp_rep_send(&hids_obj, NULL, INPUT_REP_IDX, buf, ARRAY_SIZE(buf), NULL);

    if (ret != 0) {
        LOG_ERR("ble_hid_next err: %d", ret);
        return ret;
    }

    send_key_release();

    return ret;
}

int ble_hid_previous(void)
{
    int ret = 0;

    // Byte 0: Modifier
    // Byte 1: Reserved
    // Byte 2: Keypress
    int8_t buf[REPORT_SIZE] = {0x00, 0x00, HID_KEY_LEFT_ARROW};
    // ret = bt_gatt_notify(NULL, &hog_svc.attrs[5], report, sizeof(report));
    ret = bt_hids_inp_rep_send(&hids_obj, NULL, INPUT_REP_IDX, buf, ARRAY_SIZE(buf), NULL);

    if (ret != 0) {
        LOG_ERR("ble_hid_previous err: %d", ret);
        return ret;
    }

    send_key_release();

    return ret;
}

int ble_hid_volume_up()
{
    int ret;

    uint8_t buf[REPORT_SIZE] = {HID_USAGE_VOLUME_UP};
    ret = bt_hids_inp_rep_send(&hids_obj, NULL, INPUT_REP_IDX, buf, ARRAY_SIZE(buf), NULL);

    if (ret != 0) {
        LOG_ERR("ble_hid_volume_up err: %d", ret);
        return ret;
    }

    memset(buf, 0, sizeof(buf));
    ret = bt_hids_inp_rep_send(&hids_obj, NULL, INPUT_REP_IDX, buf, ARRAY_SIZE(buf), NULL);

    if (ret != 0) {
        LOG_ERR("ble_hid_volume_up release err: %d", ret);
    }

    return ret;
}
