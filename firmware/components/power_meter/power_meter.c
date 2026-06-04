/*
 * power_meter.c — Power metering abstraction implementation
 *
 * BACKEND STRATEGY:
 *   Try #1: ucukertz/idf-hlw8032  (open-source, well-tested)
 *   Try #2: minimal built-in parser  (stopgap, replace with #1 ASAP)
 *
 * To switch to the open-source backend:
 *   1. Clone: git clone https://github.com/ucukertz/IDF-HLW8032 \
 *             firmware/components/idf-hlw8032
 *   2. Uncomment #define USE_IDF_HLW8032 below
 *   3. Rebuild: idf.py build
 */

#include "power_meter.h"
#include <string.h>
#include "driver/uart.h"
#include "esp_log.h"
#include "app_config.h"

static const char *TAG = "power_meter";

/* ================================================================
 * Backend selection
 * ================================================================ */

/*
 * Uncomment this line AFTER cloning ucukertz/idf-hlw8032 into
 * firmware/components/idf-hlw8032/ or adding it as a managed component.
 *
 * The open-source driver provides:
 *   - hlw8032_init_uart(uart_num, tx_pin, rx_pin)
 *   - hlw8032_read_data(&voltage, &current, &power, &energy, &pf, &freq)
 *   - Proper frame sync, CRC validation, calibration support
 */
/* #define USE_IDF_HLW8032 */

#ifdef USE_IDF_HLW8032
/* ---- Open-source backend (PREFERRED) ---- */
#include "hlw8032.h"

#include "hlw8032.h"

static float s_energy_wh = 0.0f;

esp_err_t power_meter_init(void) {
    ESP_LOGI(TAG, "Using open-source backend: ucukertz/idf-hlw8032");
    hlw8032_init_uart(PM_UART_PORT, PM_UART_TX_IO, PM_UART_RX_IO);
    return ESP_OK;
}

esp_err_t power_meter_read(power_data_t *data) {
    uint32_t v_reg, c_reg, p_reg, e_reg, pf_reg, freq;

    if (hlw8032_read_data(&v_reg, &c_reg, &p_reg, &e_reg, &pf_reg, &freq) != 0) {
        return ESP_FAIL;
    }

    /* Convert raw registers to engineering units using idf-hlw8032 constants */
    data->voltage       = v_reg * 0.000581f;
    data->current       = c_reg * 0.0000726f;
    data->power         = p_reg * 0.0000422f;
    data->power_factor  = pf_reg / 1000.0f;
    data->frequency     = (float)freq;

    /* Energy accumulation */
    float hours = (float)PM_READ_INTERVAL_MS / 3600000.0f;
    s_energy_wh += data->power * hours;
    data->energy_wh = s_energy_wh;

    return ESP_OK;
}

void power_meter_reset_energy(void) {
    s_energy_wh = 0.0f;
}

bool power_meter_is_calibrated(void) {
    return true;  /* idf-hlw8032 handles calibration internally */
}

#else
/* ---- Minimal built-in parser (STOPGAP — switch to USE_IDF_HLW8032) ---- */

#define HLW8032_FRAME_LEN  24
#define HLW8032_SYNC_BYTE  0x5A

static RTC_DATA_ATTR float s_energy_wh = 0.0f;
static power_data_t s_last_valid = {0};

esp_err_t power_meter_init(void) {
    ESP_LOGW(TAG, "=== USING BUILT-IN HLW8032 PARSER (STOPGAP) ===");
    ESP_LOGW(TAG, "To switch to the open-source driver:");
    ESP_LOGW(TAG, "  1. git clone https://github.com/ucukertz/IDF-HLW8032 \\");
    ESP_LOGW(TAG, "     firmware/components/idf-hlw8032");
    ESP_LOGW(TAG, "  2. Uncomment #define USE_IDF_HLW8032 in power_meter.c");
    ESP_LOGW(TAG, "  3. Rebuild: idf.py build");
    return ESP_OK;
}

static inline uint32_t u24(const uint8_t *b, int off) {
    return ((uint32_t)b[off] << 16) | ((uint32_t)b[off+1] << 8) | b[off+2];
}

static inline uint16_t u16(const uint8_t *b, int off) {
    return ((uint16_t)b[off] << 8) | b[off+1];
}

esp_err_t power_meter_read(power_data_t *data) {
    uint8_t buf[HLW8032_FRAME_LEN];

    uart_flush_input(PM_UART_PORT);

    /* Sync to 0x5A */
    int tries = 0;
    while (tries < 100) {
        uint8_t b;
        if (uart_read_bytes(PM_UART_PORT, &b, 1, pdMS_TO_TICKS(20)) == 1 && b == HLW8032_SYNC_BYTE) {
            buf[0] = b;
            break;
        }
        tries++;
    }
    if (tries >= 100) return ESP_FAIL;

    /* Read remaining 23 bytes */
    int got = 1;
    while (got < HLW8032_FRAME_LEN) {
        int n = uart_read_bytes(PM_UART_PORT, buf + got, HLW8032_FRAME_LEN - got, pdMS_TO_TICKS(100));
        if (n > 0) got += n;
        else return ESP_FAIL;
    }

    /* Parse */
    uint32_t v_raw = u24(buf, 2);
    uint32_t c_raw = u24(buf, 5);
    uint32_t p_raw = u24(buf, 8);
    uint16_t pf_raw = u16(buf, 14);
    uint8_t  f_raw  = buf[19];

    /* Approximate conversion (needs per-PCB calibration!) */
    data->voltage       = v_raw * 0.000581f;
    data->current       = c_raw * 0.0000726f;
    data->power         = p_raw * 0.0000422f;
    data->power_factor  = pf_raw / 1000.0f;
    data->frequency     = (float)f_raw;

    float hours = (float)PM_READ_INTERVAL_MS / 3600000.0f;
    s_energy_wh += data->power * hours;
    data->energy_wh = s_energy_wh;
    data->timestamp = 0;

    memcpy(&s_last_valid, data, sizeof(power_data_t));
    return ESP_OK;
}

void power_meter_reset_energy(void) { s_energy_wh = 0.0f; }
bool power_meter_is_calibrated(void) { return s_last_valid.voltage > 10.0f; }

#endif /* USE_IDF_HLW8032 */
