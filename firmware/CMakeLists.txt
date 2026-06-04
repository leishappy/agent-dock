# Top-level CMakeLists.txt for BLE Modular Power Strip firmware
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Extra component directories
set(EXTRA_COMPONENT_DIRS
    ${CMAKE_SOURCE_DIR}/components
)

project(ble_power_strip)
