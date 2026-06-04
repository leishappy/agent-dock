/*
 * ble_gatt.c — NimBLE GATT server implementation
 *
 * Based on the official ESP-IDF NimBLE_GATT_Server example:
 *   https://github.com/espressif/esp-idf/tree/master/examples/bluetooth/
 *   ble_get_started/nimble/NimBLE_GATT_Server
 *
 * Key differences from the example:
 *   - Uses custom 128-bit UUID service (Power Strip Service)
 *   - Multiple characteristics with Read/Write/Notify/Indicate
 *   - Integrates with the app's power_data_t and module_info_t types
 */

#include "ble_gatt.h"
#include <string.h>
#include "esp_log.h"
#include "esp_bt.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "app_config.h"

static const char *TAG = "ble_gatt";

/* ================================================================
 * UUID Definitions
 * ================================================================ */

/* Power Strip Service (custom 128-bit) */
#define GATT_SVR_SVC_PS_UUID              0xA000
#define GATT_SVR_CHR_MODULE_STATUS_UUID    0xA001
#define GATT_SVR_CHR_POWER_READINGS_UUID   0xA002
#define GATT_SVR_CHR_SOCKET_CONTROL_UUID   0xA003
#define GATT_SVR_CHR_NOTIFICATION_UUID     0xA004
#define GATT_SVR_CHR_DISPLAY_CONFIG_UUID   0xA005
#define GATT_SVR_CHR_SYSTEM_COMMAND_UUID   0xA006
#define GATT_SVR_CHR_MODULE_INFO_UUID      0xA007
#define GATT_SVR_CHR_TIME_SYNC_UUID        0xA008

/* Custom service base UUID */
static const ble_uuid128_t g_ps_svc_uuid = BLE_UUID128_INIT(
    0x1e, 0x0d, 0x9c, 0x8b, 0x5a, 0x3c, 0x7e, 0x9c,
    0x9a, 0x4b, 0xb2, 0xa0, 0x00, 0xa0, 0xe8, 0xd4);

/* ================================================================
 * Connection state
 * ================================================================ */

static bool s_connected = false;
static uint16_t s_conn_handle = 0;

/* Notification enabled flags per characteristic */
static bool s_notify_power = false;
static bool s_notify_module = false;

/* Characteristic handles (assigned by NimBLE during registration) */
static uint16_t s_hdl_power_readings = 0;
static uint16_t s_hdl_module_status = 0;
static uint16_t s_hdl_socket_control = 0;
static uint16_t s_hdl_time_sync = 0;

/* ================================================================
 * Power Strip Service — Characteristic Value Buffers
 * ================================================================ */

/* Module Status: 4 bytes per slot × 4 slots = 16 bytes + header */
static uint8_t s_module_status_val[20] = {0};

/* Power Readings: structured binary format */
static uint8_t s_power_readings_val[20] = {0};

/* ================================================================
 * Device Information Service (Standard 0x180A)
 *
 * These are READ-ONLY characteristics exposed via the standard
 * DIS service. Values are set once at init.
 * ================================================================ */

static const char *s_manufacturer_name = BLE_MANUFACTURER_NAME;
static const char *s_model_number = BLE_MODEL_NUMBER;
static const char *s_serial_number = "00000001";
static const char *s_hw_revision = "1.0";
static const char *s_fw_revision = "1.0.0";

/* ================================================================
 * GATT Access Callback — handles all Read/Write operations
 *
 * This is the central callback for ALL characteristics in ALL services.
 * We dispatch based on characteristic UUID.
 * ================================================================ */

static int gatt_svr_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg) {
    uint16_t uuid = ble_uuid_u16(ctxt->chr->uuid);
    int rc;

    switch (uuid) {
    /* ---- Power Readings: Read + Notify ---- */
    case GATT_SVR_CHR_POWER_READINGS_UUID:
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            rc = os_mbuf_append(ctxt->om, s_power_readings_val, sizeof(s_power_readings_val));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        break;

    /* ---- Module Status: Read + Notify ---- */
    case GATT_SVR_CHR_MODULE_STATUS_UUID:
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            rc = os_mbuf_append(ctxt->om, s_module_status_val, sizeof(s_module_status_val));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        break;

    /* ---- Socket Control: Write + Indicate ---- */
    case GATT_SVR_CHR_SOCKET_CONTROL_UUID:
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            /* Forward to command queue */
            uint8_t cmd[4] = {0};
            size_t len = OS_MBUF_PKTLEN(ctxt->om);
            if (len >= 2) {
                os_mbuf_copydata(ctxt->om, 0, 2, cmd);
                /* cmd[0] = slot, cmd[1] = command (0=OFF, 1=ON, 2=TOGGLE) */
                extern QueueHandle_t g_command_queue;
                if (g_command_queue) {
                    xQueueSend(g_command_queue, cmd, 0);
                }
                ESP_LOGI(TAG, "Socket control: slot=%d cmd=%d", cmd[0], cmd[1]);
            }
            return 0;
        }
        break;

    /* ---- Time Sync: Write + Indicate ---- */
    case GATT_SVR_CHR_TIME_SYNC_UUID:
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            uint8_t ts_buf[7] = {0};
            size_t len = OS_MBUF_PKTLEN(ctxt->om);
            if (len >= 4) {
                os_mbuf_copydata(ctxt->om, 0, 4, ts_buf);
                uint32_t timestamp = (ts_buf[0] << 24) | (ts_buf[1] << 16) |
                                     (ts_buf[2] << 8) | ts_buf[3];
                app_set_system_time(timestamp);
                ESP_LOGI(TAG, "Time synced: %lu", timestamp);
            }
            return 0;
        }
        break;

    /* ---- System Command: Write + Indicate ---- */
    case GATT_SVR_CHR_SYSTEM_COMMAND_UUID:
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            uint8_t cmd;
            os_mbuf_copydata(ctxt->om, 0, 1, &cmd);
            ESP_LOGI(TAG, "System command: 0x%02X", cmd);
            if (cmd == 0x01) {
                ESP_LOGW(TAG, "Reset requested — rebooting in 1s...");
                vTaskDelay(pdMS_TO_TICKS(1000));
                esp_restart();
            }
            return 0;
        }
        break;

    /* ---- Manufacturer Name (DIS) ---- */
    case 0x2A29:
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            rc = os_mbuf_append(ctxt->om, s_manufacturer_name, strlen(s_manufacturer_name));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        break;

    /* ---- Model Number (DIS) ---- */
    case 0x2A24:
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            rc = os_mbuf_append(ctxt->om, s_model_number, strlen(s_model_number));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        break;

    /* ---- Serial Number (DIS) ---- */
    case 0x2A25:
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            rc = os_mbuf_append(ctxt->om, s_serial_number, strlen(s_serial_number));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        break;

    default:
        ESP_LOGD(TAG, "Unhandled access: uuid=0x%04X op=%d", uuid, ctxt->op);
        break;
    }

    return BLE_ATT_ERR_UNLIKELY;
}

/* ================================================================
 * GATT Service Definition Table
 *
 * This is NimBLE's declarative way of defining services.
 * Each characteristic lists its UUID, flags, and min/max key sizes.
 * NimBLE auto-assigns handles during registration.
 * ================================================================ */

static const struct ble_gatt_svc_def g_svcs[] = {
    /* === Device Information Service (0x180A) === */
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(0x180A),
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = BLE_UUID16_DECLARE(0x2A29),  /* Manufacturer Name */
                .access_cb = gatt_svr_access_cb,
                .flags = BLE_GATT_CHR_F_READ,
            },
            {
                .uuid = BLE_UUID16_DECLARE(0x2A24),  /* Model Number */
                .access_cb = gatt_svr_access_cb,
                .flags = BLE_GATT_CHR_F_READ,
            },
            {
                .uuid = BLE_UUID16_DECLARE(0x2A25),  /* Serial Number */
                .access_cb = gatt_svr_access_cb,
                .flags = BLE_GATT_CHR_F_READ,
            },
            {
                .uuid = BLE_UUID16_DECLARE(0x2A26),  /* Firmware Revision */
                .access_cb = gatt_svr_access_cb,
                .flags = BLE_GATT_CHR_F_READ,
            },
            {
                .uuid = 0,  /* Sentinel */
            }
        },
    },

    /* === Power Strip Service (custom 128-bit UUID) === */
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = (ble_uuid_t *)&g_ps_svc_uuid,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                /* Module Status — Read + Notify */
                .uuid = BLE_UUID16_DECLARE(GATT_SVR_CHR_MODULE_STATUS_UUID),
                .access_cb = gatt_svr_access_cb,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
            },
            {
                /* Power Readings — Read + Notify */
                .uuid = BLE_UUID16_DECLARE(GATT_SVR_CHR_POWER_READINGS_UUID),
                .access_cb = gatt_svr_access_cb,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
            },
            {
                /* Socket Control — Write + Indicate */
                .uuid = BLE_UUID16_DECLARE(GATT_SVR_CHR_SOCKET_CONTROL_UUID),
                .access_cb = gatt_svr_access_cb,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_INDICATE,
            },
            {
                /* Notification Push — Write */
                .uuid = BLE_UUID16_DECLARE(GATT_SVR_CHR_NOTIFICATION_UUID),
                .access_cb = gatt_svr_access_cb,
                .flags = BLE_GATT_CHR_F_WRITE,
            },
            {
                /* Display Config — Write */
                .uuid = BLE_UUID16_DECLARE(GATT_SVR_CHR_DISPLAY_CONFIG_UUID),
                .access_cb = gatt_svr_access_cb,
                .flags = BLE_GATT_CHR_F_WRITE,
            },
            {
                /* System Command — Write + Indicate */
                .uuid = BLE_UUID16_DECLARE(GATT_SVR_CHR_SYSTEM_COMMAND_UUID),
                .access_cb = gatt_svr_access_cb,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_INDICATE,
            },
            {
                /* Module Info — Read + Notify */
                .uuid = BLE_UUID16_DECLARE(GATT_SVR_CHR_MODULE_INFO_UUID),
                .access_cb = gatt_svr_access_cb,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
            },
            {
                /* Time Sync — Write + Indicate */
                .uuid = BLE_UUID16_DECLARE(GATT_SVR_CHR_TIME_SYNC_UUID),
                .access_cb = gatt_svr_access_cb,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_INDICATE,
            },
            {
                .uuid = 0,  /* Sentinel */
            }
        },
    },

    {
        .type = 0,  /* Sentinel: end of service list */
    },
};

/* ================================================================
 * BLE Event Callbacks
 *
 * NimBLE delivers events via this callback.
 * We track connection state and subscription status here.
 * ================================================================ */

static int gatt_svr_event_cb(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            ESP_LOGI(TAG, "BLE client connected");
            s_connected = true;
            s_conn_handle = event->connect.conn_handle;
        } else {
            ESP_LOGW(TAG, "BLE connection failed: %d", event->connect.status);
            s_connected = false;
            /* Resume advertising */
            ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, NULL,
                              gatt_svr_event_cb, NULL);
        }
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "BLE client disconnected (reason=%d)", event->disconnect.reason);
        s_connected = false;
        s_notify_power = false;
        s_notify_module = false;
        /* Resume advertising */
        ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, NULL,
                          gatt_svr_event_cb, NULL);
        return 0;

    case BLE_GAP_EVENT_SUBSCRIBE:
        if (event->subscribe.attr_handle == s_hdl_power_readings) {
            s_notify_power = (event->subscribe.cur_notify > 0);
            ESP_LOGI(TAG, "Power Readings notify: %s", s_notify_power ? "ON" : "OFF");
        } else if (event->subscribe.attr_handle == s_hdl_module_status) {
            s_notify_module = (event->subscribe.cur_notify > 0);
            ESP_LOGI(TAG, "Module Status notify: %s", s_notify_module ? "ON" : "OFF");
        }
        return 0;

    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(TAG, "MTU updated: %d", event->mtu.value);
        return 0;

    default:
        return 0;
    }
}

/* ================================================================
 * Advertising Configuration
 * ================================================================ */

static void ble_gatt_advertise(void) {
    struct ble_gap_adv_params adv_params = {0};
    struct ble_hs_adv_fields fields = {0};

    /* Advertise as a connectable peripheral */
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.tx_pwr_lvl_is_present = false;
    fields.name = (uint8_t *)BLE_DEVICE_NAME;
    fields.name_len = strlen(BLE_DEVICE_NAME);
    fields.name_is_complete = 1;

    /* Include the Power Strip Service UUID in advertising data
     * so Web Bluetooth can filter by service. */
    fields.uuids128 = (ble_uuid128_t *)&g_ps_svc_uuid;
    fields.num_uuids128 = 1;
    fields.uuids128_is_complete = 1;

    int rc = ble_gap_adv_set_fields(&fields);
    assert(rc == 0);

    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                           &adv_params, gatt_svr_event_cb, NULL);
    assert(rc == 0);
    ESP_LOGI(TAG, "Advertising started as '%s'", BLE_DEVICE_NAME);
}

/* ================================================================
 * NimBLE Host Task
 *
 * NimBLE runs its own FreeRTOS task (handled by nimble_port_freertos).
 * We just need to call nimble_port_run() once.
 * ================================================================ */

static void nimble_host_task(void *param) {
    ESP_LOGI(TAG, "NimBLE host task started");
    nimble_port_run();  /* This function never returns */
}

/* ================================================================
 * Initialization
 * ================================================================ */

esp_err_t ble_gatt_init(void) {
    int rc;

    ESP_LOGI(TAG, "Initializing NimBLE GATT server...");

    /* Initialize NimBLE porting layer */
    nimble_port_init();
    ESP_LOGI(TAG, "NimBLE port initialized");

    /* Configure GAP device name and appearance */
    ble_svc_gap_device_name_set(BLE_DEVICE_NAME);
    /* Appearance: Generic Power Outlet (0x0310) or Generic (0x0000) */
    ble_svc_gap_device_appearance_set(0x0310);

    /* Register GATT services (device info + power strip service) */
    rc = ble_gatts_count_cfg(g_svcs);
    assert(rc == 0);
    rc = ble_gatts_add_svcs(g_svcs);
    assert(rc == 0);
    ESP_LOGI(TAG, "GATT services registered: 0x180A + PowerStrip");

    /* Look up characteristic handles for later notification */
    /* (Handles are assigned by NimBLE — we find them by UUID) */
    /* For simplicity, we don't look up handles here.
     * In a production app, use ble_gatts_find_chr() or store handles
     * after registration. The SUBSCRIBE event delivers the handle. */

    /* Initialize the NimBLE host task */
    nimble_port_freertos_init(nimble_host_task);
    ESP_LOGI(TAG, "NimBLE host task created");

    /* Start advertising */
    ble_gatt_advertise();

    ESP_LOGI(TAG, "BLE GATT server ready");
    ESP_LOGI(TAG, "Service UUID: d4e8a000-a0b2-4b9a-9c7e-3c5a8b9c0d1e");

    return ESP_OK;
}

/* ================================================================
 * Notification Helpers
 * ================================================================ */

esp_err_t ble_gatt_notify_power(const power_data_t *data) {
    if (!s_connected || !s_notify_power) return ESP_OK;

    /* Pack power data into 20-byte binary format
     * See plan document §5 for format specification */
    memset(s_power_readings_val, 0, sizeof(s_power_readings_val));

    uint16_t v = (uint16_t)(data->voltage * 10);    /* 220.0V → 2200 */
    uint16_t c = (uint16_t)(data->current * 1000);  /* 1.5A → 1500 */
    uint32_t w = (uint32_t)(data->power * 1000);    /* 330W → 330000 */
    uint32_t wh = (uint32_t)(data->energy_wh);

    s_power_readings_val[0] = (v >> 8) & 0xFF;
    s_power_readings_val[1] = v & 0xFF;
    s_power_readings_val[2] = (c >> 8) & 0xFF;
    s_power_readings_val[3] = c & 0xFF;
    s_power_readings_val[4] = (w >> 24) & 0xFF;
    s_power_readings_val[5] = (w >> 16) & 0xFF;
    s_power_readings_val[6] = (w >> 8) & 0xFF;
    s_power_readings_val[7] = w & 0xFF;
    s_power_readings_val[8] = (wh >> 24) & 0xFF;
    s_power_readings_val[9] = (wh >> 16) & 0xFF;
    s_power_readings_val[10] = (wh >> 8) & 0xFF;
    s_power_readings_val[11] = wh & 0xFF;
    s_power_readings_val[12] = (uint8_t)(data->power_factor * 100);
    s_power_readings_val[13] = (uint8_t)(data->frequency * 10);

    /* NimBLE notification: use the GATT characteristic access to send notification */
    struct os_mbuf *om = ble_hs_mbuf_from_flat(s_power_readings_val, sizeof(s_power_readings_val));
    if (om) {
        int rc = ble_gattc_notify_custom(s_conn_handle, s_hdl_power_readings, om);
        if (rc != 0) {
            ESP_LOGW(TAG, "Power notify failed: %d", rc);
        }
    }
    return ESP_OK;
}

esp_err_t ble_gatt_notify_module(uint8_t slot, bool present, uint8_t mod_type) {
    if (!s_connected || !s_notify_module) return ESP_OK;
    if (slot >= NUM_SLOTS) return ESP_ERR_INVALID_ARG;

    /* Update the module status buffer */
    size_t offset = 1 + slot * 4;
    s_module_status_val[offset] = mod_type;
    s_module_status_val[offset + 1] = present ? 0x01 : 0x00;

    struct os_mbuf *om = ble_hs_mbuf_from_flat(s_module_status_val, sizeof(s_module_status_val));
    if (om) {
        int rc = ble_gattc_notify_custom(s_conn_handle, s_hdl_module_status, om);
        if (rc != 0) {
            ESP_LOGW(TAG, "Module notify failed: %d", rc);
        }
    }
    return ESP_OK;
}

bool ble_gatt_is_connected(void) {
    return s_connected;
}
