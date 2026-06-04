/*
 * power_meter.h — Power metering abstraction layer
 *
 * This module provides a unified API for reading power data.
 *
 * PREFERRED BACKEND: ucukertz/idf-hlw8032 (open-source ESP-IDF component)
 *   GitHub: https://github.com/ucukertz/IDF-HLW8032
 *   Install: git clone into firmware/components/idf-hlw8032/
 *            OR add git dependency in firmware/main/idf_component.yml
 *
 * The API is designed so swapping backends requires NO changes to
 * any other file — only power_meter.c needs to change.
 */

#ifndef POWER_METER_H
#define POWER_METER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "app_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the power meter subsystem.
 *
 * Configures UART and prepares the HLW8032 for continuous reading.
 * Must be called once after board_init().
 *
 * @return ESP_OK on success
 */
esp_err_t power_meter_init(void);

/**
 * @brief Read one power measurement.
 *
 * Blocks until a valid measurement is obtained (typically ~200ms).
 * If the HLW8032 is not responding, returns ESP_FAIL.
 *
 * @param data  Output struct filled with V, A, W, kWh, PF, Hz
 * @return ESP_OK on valid reading, ESP_FAIL on error
 */
esp_err_t power_meter_read(power_data_t *data);

/**
 * @brief Reset the accumulated energy counter to zero.
 */
void power_meter_reset_energy(void);

/**
 * @brief Check if the meter is returning valid data.
 */
bool power_meter_is_calibrated(void);

#ifdef __cplusplus
}
#endif

#endif /* POWER_METER_H */
