/*
 * ble_gatt.h — BLE GATT server for BLE Modular Power Strip
 *
 * Implements the Power Strip Service and Device Information Service
 * using NimBLE (the recommended BLE stack for ESP-IDF).
 *
 * Reference: ESP-IDF official example
 *   $IDF_PATH/examples/bluetooth/ble_get_started/nimble/NimBLE_GATT_Server/
 *
 * Service UUID: d4e8a000-a0b2-4b9a-9c7e-3c5a8b9c0d1e
 */

#ifndef BLE_GATT_H
#define BLE_GATT_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize and start the BLE GATT server.
 *
 * Sets up NimBLE host, configures Device Information Service
 * and Power Strip Service, starts advertising.
 *
 * Must be called after NVS init and board_init().
 */
esp_err_t ble_gatt_init(void);

/**
 * @brief Notify connected client of updated power readings.
 *
 * Sends a BLE notification on the Power Readings characteristic
 * (d4e8a002). Safe to call even if no client is connected.
 *
 * @param data  Current power measurement
 * @return ESP_OK if notification was queued successfully
 */
esp_err_t ble_gatt_notify_power(const struct power_data *data);

/**
 * @brief Notify connected client of module status change.
 *
 * @param slot      0-based slot index
 * @param present   true if module inserted
 * @param mod_type  module type (module_type_t enum)
 */
esp_err_t ble_gatt_notify_module(uint8_t slot, bool present, uint8_t mod_type);

/**
 * @brief Check if a BLE client is currently connected.
 */
bool ble_gatt_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif /* BLE_GATT_H */
