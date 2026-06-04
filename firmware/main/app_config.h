/*
 * app_config.h — Board-level pin definitions and system constants
 * BLE Modular Power Strip
 * ESP32-S3-WROOM-1 N16R8
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================
 * GPIO Pin Assignments
 * ================================================================ */

/* I2C bus for module identification (I2C0) */
#define I2C_MASTER_SCL_IO          GPIO_NUM_1
#define I2C_MASTER_SDA_IO          GPIO_NUM_2
#define I2C_MASTER_FREQ_HZ         100000      /* 100 kHz standard mode */
#define I2C_MASTER_PORT            I2C_NUM_0

/* Module bay interrupt pins (one per slot, active low, internal pull-up) */
#define INT_SLOT1_IO               GPIO_NUM_3
#define INT_SLOT2_IO               GPIO_NUM_4
#define INT_SLOT3_IO               GPIO_NUM_5
#define INT_SLOT4_IO               GPIO_NUM_6

/* Relay control pins (one per slot, high = ON) */
#define RELAY_SLOT1_IO             GPIO_NUM_7
#define RELAY_SLOT2_IO             GPIO_NUM_15
#define RELAY_SLOT3_IO             GPIO_NUM_16
#define RELAY_SLOT4_IO             GPIO_NUM_17

/* HLW8032 power meter UART */
#define PM_UART_RX_IO              GPIO_NUM_8   /* ESP RX <- HLW TX */
#define PM_UART_TX_IO              GPIO_NUM_18  /* ESP TX -> HLW RX (unused) */
#define PM_UART_PORT               UART_NUM_1
#define PM_UART_BAUD               9600
#define PM_UART_RX_BUF_SIZE        256

/* Display SPI (ST7789, Phase 2+) */
#define DISP_SPI_MOSI_IO           GPIO_NUM_9
#define DISP_SPI_SCLK_IO           GPIO_NUM_10
#define DISP_SPI_CS_IO             GPIO_NUM_11
#define DISP_SPI_DC_IO             GPIO_NUM_12
#define DISP_SPI_RST_IO            GPIO_NUM_13
#define DISP_BL_PWM_IO             GPIO_NUM_14

/* Status LED (WS2812B) */
#define STATUS_LED_IO              GPIO_NUM_35

/* Physical mode button */
#define BUTTON_MODE_IO             GPIO_NUM_47

/* Power hold (self-latching supply, Phase 3) */
#define PWR_HOLD_IO                GPIO_NUM_21

/* USB Serial JTAG (built-in) */
#define DEBUG_TX_IO                GPIO_NUM_46

/* ================================================================
 * System Constants
 * ================================================================ */

#define NUM_SLOTS                  4
#define MODULE_I2C_ADDR_BASE       0x10        /* Slot N = 0x10 + (N-1) */
#define BLE_DEVICE_NAME            "BLE-PowerStrip"
#define BLE_MANUFACTURER_NAME      "ModularPower"
#define BLE_MODEL_NUMBER           "BLE-MPS-1"

/* Power meter read interval (ms) */
#define PM_READ_INTERVAL_MS        1000

/* Module debounce time (ms) */
#define MODULE_DEBOUNCE_MS         50

/* Display refresh interval (ms), Phase 2+ */
#define DISP_REFRESH_INTERVAL_MS   20          /* 50 FPS LVGL tick */

/* Task stack sizes (words) */
#define STACK_MAIN                 4096
#define STACK_BLE                  8192
#define STACK_MODULE_MONITOR       3072
#define STACK_POWER_METER          2048
#define STACK_DISPLAY              4096
#define STACK_NOTIFICATION         2048
#define STACK_BUTTON               1024

/* Task priorities */
#define PRIO_MAIN                  5
#define PRIO_BLE                   4
#define PRIO_MODULE_MONITOR        3
#define PRIO_POWER_METER           2
#define PRIO_DISPLAY               2
#define PRIO_NOTIFICATION          1
#define PRIO_BUTTON                1

/* ================================================================
 * Module Type Enumeration
 * ================================================================ */

typedef enum {
    MODULE_TYPE_EMPTY           = 0x00,
    MODULE_TYPE_CHINESE_5HOLE   = 0x01,  /* AC socket (10A rated) */
    MODULE_TYPE_USB_A_C         = 0x02,  /* USB-A + USB-C charging */
    MODULE_TYPE_SENSOR_TEMP     = 0x03,  /* Temperature/humidity sensor */
    MODULE_TYPE_SENSOR_LIGHT    = 0x04,  /* Ambient light sensor */
    MODULE_TYPE_SENSOR_PIR      = 0x05,  /* Motion sensor */
    MODULE_TYPE_NIGHT_LIGHT     = 0x06,  /* LED night light */
    MODULE_TYPE_POWER_MON       = 0x07,  /* Per-module power monitoring */
    MODULE_TYPE_WIRELESS_CHG    = 0x08,  /* Qi wireless charger */
    MODULE_TYPE_FAN             = 0x09,  /* USB fan module */
    MODULE_TYPE_USER_DEFINED    = 0xF0,
} module_type_t;

/* ================================================================
 * Module Info Structure (per slot)
 * ================================================================ */

typedef struct {
    module_type_t type;
    uint8_t       hw_version;
    uint8_t       fw_version;
    uint32_t      serial_num;
    uint16_t      capabilities;
    uint8_t       status;          /* 0=off, 1=on, 2=fault */
    uint8_t       fault_code;
    bool          present;         /* true if module is detected */
} module_info_t;

/* ================================================================
 * Power Data Structure
 * ================================================================ */

typedef struct {
    float voltage;                  /* Volts RMS */
    float current;                  /* Amps RMS */
    float power;                    /* Watts (active) */
    float energy_wh;                /* Accumulated Watt-hours */
    float power_factor;             /* 0.0 - 1.0 */
    float frequency;                /* Hz */
    uint32_t timestamp;             /* Unix time (from BLE time sync) */
} power_data_t;

#ifdef __cplusplus
}
#endif

#endif /* APP_CONFIG_H */
