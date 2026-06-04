/*
 * app_config.c — Global system state and configuration
 */

#include "app_config.h"

/* Global power data (updated by power_meter task, read by BLE task) */
static power_data_t g_power_data = {0};

/* Module info array (one per slot, updated by module_monitor task) */
static module_info_t g_modules[NUM_SLOTS] = {0};

/* System time (set via BLE Time Sync characteristic) */
static uint32_t g_system_time = 0;

/* ================================================================
 * Accessor functions
 * ================================================================ */

power_data_t* app_get_power_data(void) {
    return &g_power_data;
}

module_info_t* app_get_modules(void) {
    return g_modules;
}

module_info_t* app_get_module(uint8_t slot) {
    if (slot >= NUM_SLOTS) return NULL;
    return &g_modules[slot];
}

uint32_t app_get_system_time(void) {
    return g_system_time;
}

void app_set_system_time(uint32_t timestamp) {
    g_system_time = timestamp;
}
