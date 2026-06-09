// ============================================================
// display_mgr.h — TFT 显示 + 背光封装
// ============================================================
// 基于 TFT_eSPI + ILI9341 240x320 SPI
// 背光 PWM 控制 (GPIO 2, LEDC)
// ============================================================
#pragma once
#include <TFT_eSPI.h>
#include "config.h"

class DisplayMgr {
public:
    static DisplayMgr& instance();

    void begin();              // 初始化 TFT + 背光
    void setBacklight(uint8_t level);  // 0-255

    TFT_eSPI& tft() { return _tft; }

private:
    DisplayMgr() = default;

    TFT_eSPI  _tft;
};
