/*
 * main.c — BLE Modular Power Strip firmware entry point
 * Phase 1 MVP: HLW8032 power meter + BLE GATT + relay control
 *
 * Hardware: ESP32-S3 DevKit C
 *            HLW8032 on breadboard (UART)
 *            1x Relay module + AC socket + bulb load
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "app_config.h"
#include "board.h"
#include "power_meter.h"
#include "ble_gatt.h"

static const char *TAG = "main";

/* Task handles */
static TaskHandle_t s_power_meter_task = NULL;
static TaskHandle_t s_module_monitor_task = NULL;

/* Queues for inter-task communication */
QueueHandle_t g_power_data_queue = NULL;
QueueHandle_t g_command_queue = NULL;

/* ================================================================
 * Power Meter Task
 *
 * Reads HLW8032 data every PM_READ_INTERVAL_MS, publishes to queue.
 * Phase 1: logs to serial; Phase 2: also pushes to BLE notify.
 * ================================================================ */
static void power_meter_task(void *arg) {
    power_data_t data;

    ESP_LOGI(TAG, "Power meter task started");
    vTaskDelay(pdMS_TO_TICKS(2000));  /* Wait for HLW8032 to stabilize */

    while (1) {
        if (power_meter_read(&data) == ESP_OK) {
            /* Log to serial for debugging */
            ESP_LOGI(TAG, "POWER: %.1fV | %.3fA | %.1fW | PF=%.2f | %.1fHz | %.3fkWh",
                     data.voltage, data.current, data.power,
                     data.power_factor, data.frequency,
                     data.energy_wh / 1000.0f);

            /* Publish to queue for BLE task */
            if (g_power_data_queue) {
                xQueueSend(g_power_data_queue, &data, 0);
            }
        } else {
            ESP_LOGW(TAG, "Power meter read failed");
        }

        vTaskDelay(pdMS_TO_TICKS(PM_READ_INTERVAL_MS));
    }
}

/* ================================================================
 * Module Monitor Task (Phase 2 placeholder)
 *
 * Phase 1: just logs slot status
 * Phase 2: monitors INT pins, detects hot-swap, probes I2C
 * ================================================================ */
static void module_monitor_task(void *arg) {
    ESP_LOGI(TAG, "Module monitor task started (Phase 1: no-op)");

    while (1) {
        /* Phase 2: wait on interrupt semaphore, probe I2C */
        vTaskDelay(pdMS_TO_TICKS(5000));
        ESP_LOGI(TAG, "Module monitor: heartbeat (no modules detected in Phase 1)");
    }
}

/* ================================================================
 * Command Processing Task
 *
 * Processes commands from BLE writes (socket ON/OFF, etc.)
 * ================================================================ */
static void command_task(void *arg) {
    uint8_t cmd_buf[4];

    ESP_LOGI(TAG, "Command task started");

    while (1) {
        if (xQueueReceive(g_command_queue, cmd_buf, portMAX_DELAY) == pdTRUE) {
            uint8_t slot = cmd_buf[0];
            uint8_t cmd = cmd_buf[1];

            ESP_LOGI(TAG, "CMD: slot=%d cmd=%d", slot, cmd);

            if (slot >= NUM_SLOTS) {
                ESP_LOGW(TAG, "Invalid slot %d", slot);
                continue;
            }

            switch (cmd) {
            case 0x00:  /* OFF */
                board_relay_set(slot, false);
                ESP_LOGI(TAG, "Slot %d turned OFF", slot);
                break;
            case 0x01:  /* ON */
                board_relay_set(slot, true);
                ESP_LOGI(TAG, "Slot %d turned ON", slot);
                break;
            case 0x02:  /* TOGGLE */
                board_relay_toggle(slot);
                ESP_LOGI(TAG, "Slot %d TOGGLED", slot);
                break;
            default:
                ESP_LOGW(TAG, "Unknown command %d for slot %d", cmd, slot);
                break;
            }
        }
    }
}

/* ================================================================
 * Application Entry Point
 * ================================================================ */
void app_main(void) {
    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "  BLE Modular Power Strip — Firmware v1.0");
    ESP_LOGI(TAG, "  Phase 1 MVP");
    ESP_LOGI(TAG, "===========================================");

    /* Initialize NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialized");

    /* Initialize board (GPIOs, relays default OFF) */
    board_init();
    ESP_LOGI(TAG, "Board initialized");

    /* Create inter-task queues */
    g_power_data_queue = xQueueCreate(4, sizeof(power_data_t));
    g_command_queue = xQueueCreate(8, sizeof(uint8_t[4]));
    ESP_LOGI(TAG, "Queues created");

    /* Initialize and start BLE GATT server */
    ble_gatt_init();
    ESP_LOGI(TAG, "BLE GATT server initialized");

    /* Create FreeRTOS tasks */
    BaseType_t task_ok;

    task_ok = xTaskCreatePinnedToCore(
        power_meter_task, "power_meter",
        STACK_POWER_METER, NULL,
        PRIO_POWER_METER, &s_power_meter_task, 0);
    assert(task_ok == pdPASS);

    task_ok = xTaskCreatePinnedToCore(
        command_task, "command",
        STACK_NOTIFICATION, NULL,
        PRIO_NOTIFICATION, NULL, 0);
    assert(task_ok == pdPASS);

    task_ok = xTaskCreatePinnedToCore(
        module_monitor_task, "module_mon",
        STACK_MODULE_MONITOR, NULL,
        PRIO_MODULE_MONITOR, &s_module_monitor_task, 1);
    assert(task_ok == pdPASS);

    ESP_LOGI(TAG, "All tasks created — system running");
    ESP_LOGI(TAG, "BLE device name: %s", BLE_DEVICE_NAME);
    ESP_LOGI(TAG, "Waiting for BLE connection...");

    /* Delete main task — we're done initializing */
    vTaskDelete(NULL);
}
