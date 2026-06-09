// ============================================================
// display_mgr.cpp — TFT 显示 + 背光封装
// ============================================================
#include "display_mgr.h"

DisplayMgr& DisplayMgr::instance() {
    static DisplayMgr mgr;
    return mgr;
}

void DisplayMgr::begin() {
    Serial.println("[Display] TFT init...");
    _tft.init();
    Serial.println("[Display] TFT init 完成");

    _tft.setRotation(0);
    _tft.setSwapBytes(true);
    _tft.fillScreen(Color::BG);

    Serial.printf("[Display] %dx%d, 就绪\n", SCREEN_WIDTH, SCREEN_HEIGHT);
}

void DisplayMgr::setBacklight(uint8_t level) {
    ledcSetup(0, 5000, 8);
    ledcAttachPin(TFT_BL, 0);
    ledcWrite(0, level);
}
