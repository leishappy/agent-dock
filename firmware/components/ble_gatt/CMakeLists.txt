idf_component_register(
    SRCS
        "ble_gatt.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        nvs_flash
        driver
)

# NimBLE is part of ESP-IDF framework — no extra dependency needed.
# Enable via sdkconfig: CONFIG_BT_ENABLED=y, CONFIG_BT_NIMBLE_ENABLED=y
#
# For a more full-featured BLE GATT server, consider:
#   espp/ble_gatt_server (ESP Component Registry)
#   idf.py add-dependency "espp/ble_gatt_server"
