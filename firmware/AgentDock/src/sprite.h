// ============================================================
// sprite.h — 像素角色动画
// ============================================================
#pragma once
#include <TFT_eSPI.h>

class Sprite {
public:
    static uint32_t draw(TFT_eSPI& tft, int x, int y, int w, int h,
                         const char* status, uint32_t frameMs);
};
