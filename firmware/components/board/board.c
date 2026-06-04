/*
 * board.c — Board initialization and GPIO abstraction
 * Phase 1: ESP32-S3 DevKit C, 1 relay, wired directly
 * Phase 2: Custom PCB with 4 relays + I2C + display
 */

#include "board.h"
#include <stdio.h>
#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "app_config.h"

static const char *TAG = "board";

/* Relay GPIO array (mapped by slot) */
static const gpio_num_t s_relay_pins[NUM_SLOTS] = {
    RELAY_SLOT1_IO,
    RELAY_SLOT2_IO,
    RELAY_SLOT3_IO,
    RELAY_SLOT4_IO,
};

/* INT GPIO array (mapped by slot) */
static const gpio_num_t s_int_pins[NUM_SLOTS] = {
    INT_SLOT1_IO,
    INT_SLOT2_IO,
    INT_SLOT3_IO,
    INT_SLOT4_IO,
};

/* Current relay states */
static bool s_relay_states[NUM_SLOTS] = {false};

/* ================================================================
 * GPIO Configuration
 * ================================================================ */

static void gpio_init(void) {
    /* Relay pins: output, default low (OFF) */
    for (int i = 0; i < NUM_SLOTS; i++) {
        gpio_config_t relay_conf = {
            .pin_bit_mask = (1ULL << s_relay_pins[i]),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLDOWN_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_ENABLE,  /* Pull down → safe OFF */
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&relay_conf);
        gpio_set_level(s_relay_pins[i], 0);
    }

    /* INT pins: input, internal pull-up (active low) */
    for (int i = 0; i < NUM_SLOTS; i++) {
        gpio_config_t int_conf = {
            .pin_bit_mask = (1ULL << s_int_pins[i]),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,  /* Phase 2: enable edge interrupts */
        };
        gpio_config(&int_conf);
    }

    /* Mode button */
    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BUTTON_MODE_IO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&btn_conf);

    /* Status LED (WS2812B) — output, off by default */
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << STATUS_LED_IO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLDOWN_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);
    gpio_set_level(STATUS_LED_IO, 0);

    ESP_LOGI(TAG, "GPIO initialized: %d relays, %d INT pins", NUM_SLOTS, NUM_SLOTS);
}

/* ================================================================
 * UART Configuration (for HLW8032 power meter)
 * ================================================================ */

static void uart_init(void) {
    uart_config_t uart_conf = {
        .baud_rate = PM_UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(
        PM_UART_PORT, PM_UART_RX_BUF_SIZE, 0, 0, NULL, 0));

    ESP_ERROR_CHECK(uart_param_config(PM_UART_PORT, &uart_conf));
    ESP_ERROR_CHECK(uart_set_pin(
        PM_UART_PORT, PM_UART_TX_IO, PM_UART_RX_IO,
        UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "UART initialized: baud=%d RX=%d TX=%d",
             PM_UART_BAUD, PM_UART_RX_IO, PM_UART_TX_IO);
}

/* ================================================================
 * I2C Configuration (for module identification, Phase 2+)
 * ================================================================ */

static void i2c_init(void) {
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_PORT, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(
        I2C_MASTER_PORT, I2C_MODE_MASTER, 0, 0, 0));

    ESP_LOGI(TAG, "I2C initialized: SDA=%d SCL=%d freq=%d Hz",
             I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, I2C_MASTER_FREQ_HZ);
}

/* ================================================================
 * Public API
 * ================================================================ */

void board_init(void) {
    ESP_LOGI(TAG, "Initializing board...");
    gpio_init();
    uart_init();
    i2c_init();
    ESP_LOGI(TAG, "Board initialization complete");
}

void board_relay_set(uint8_t slot, bool on) {
    if (slot >= NUM_SLOTS) return;
    gpio_set_level(s_relay_pins[slot], on ? 1 : 0);
    s_relay_states[slot] = on;
}

void board_relay_toggle(uint8_t slot) {
    if (slot >= NUM_SLOTS) return;
    bool new_state = !s_relay_states[slot];
    board_relay_set(slot, new_state);
}

bool board_relay_get_state(uint8_t slot) {
    if (slot >= NUM_SLOTS) return false;
    return s_relay_states[slot];
}

bool board_slot_int_read(uint8_t slot) {
    if (slot >= NUM_SLOTS) return true;  /* HIGH = no module */
    return gpio_get_level(s_int_pins[slot]);
}
