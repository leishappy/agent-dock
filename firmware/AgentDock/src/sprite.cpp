// ============================================================
// sprite.cpp — 像素角色动画实现
// ============================================================
#include "sprite.h"
#include "config.h"

// ---- 绘图辅助 -------------------------------------------------
static void drawBody(TFT_eSPI& t, int cx, int cy, int r, uint16_t c) {
    t.fillCircle(cx, cy, r, c);
}
static void drawFace(TFT_eSPI& t, int cx, int cy, int r, uint16_t c) {
    t.fillCircle(cx, cy, r, c);
    t.fillCircle(cx - r/3, cy - r/3, r/6, TFT_WHITE); // 高光
}

// ---- 空闲: 眨眼 ------------------------------------------------
static uint32_t drawIdle(TFT_eSPI& t, int x, int y, int s, uint32_t ms) {
    int cx = x + s/2, cy = y + s/2, r = s/2 - 2;
    t.fillRect(x, y, s, s, Color::BG);
    drawFace(t, cx, cy, r, Color::ACCENT);

    bool blink = (ms % 2000) < 120;
    if (blink) {
        t.fillRect(cx - 8, cy - 4, 6, 2, TFT_BLACK);
        t.fillRect(cx + 2, cy - 4, 6, 2, TFT_BLACK);
    } else {
        t.fillCircle(cx - 5, cy - 3, 3, TFT_BLACK);
        t.fillCircle(cx + 5, cy - 3, 3, TFT_BLACK);
    }
    t.drawLine(cx - 4, cy + 6, cx + 4, cy + 6, TFT_BLACK);
    t.drawLine(cx - 3, cy + 7, cx + 3, cy + 7, TFT_BLACK);

    // 耳朵
    t.fillTriangle(cx-r+4, cy-r+4, cx-r+12, cy-r-4, cx-r+14, cy-r+8, Color::ACCENT);
    t.fillTriangle(cx+r-4, cy-r+4, cx+r-12, cy-r-4, cx+r-14, cy-r+8, Color::ACCENT);
    return 200;
}

// ---- 思考: 眼珠转圈 --------------------------------------------
static uint32_t drawThink(TFT_eSPI& t, int x, int y, int s, uint32_t ms) {
    int cx = x + s/2, cy = y + s/2, r = s/2 - 2;
    uint16_t body = 0xFD80;
    t.fillRect(x, y, s, s, Color::BG);
    drawFace(t, cx, cy, r, body);

    int phase = (ms / 300) % 4;
    int dx[] = {0, 3, 0, -3}, dy[] = {-3, 0, 0, -3};
    t.fillCircle(cx-5 + dx[phase], cy-4 + dy[phase], 2, TFT_BLACK);
    t.fillCircle(cx+5 + dx[(phase+1)%4], cy-4 + dy[(phase+1)%4], 2, TFT_BLACK);

    t.fillCircle(cx, cy + 5, 3, TFT_BLACK); // 嘴

    // 问号
    int qx = cx + r/2 + 2, qy = cy - r/2 - 10;
    t.drawLine(qx, qy+10, qx, qy, TFT_WHITE);
    t.drawLine(qx, qy, qx + 4, qy, TFT_WHITE);
    t.fillCircle(qx, qy - 2, 2, TFT_WHITE);
    return 250;
}

// ---- 工具: 小锤子 ----------------------------------------------
static uint32_t drawTool(TFT_eSPI& t, int x, int y, int s, uint32_t ms) {
    int cx = x + s/2, cy = y + s/2, r = s/2 - 2;
    uint16_t body = 0xFD60;
    t.fillRect(x, y, s, s, Color::BG);
    drawBody(t, cx, cy, r, body);

    int shake = ((ms / 100) % 3) - 1;
    t.fillCircle(cx-6 + shake*2, cy-4, 2, TFT_BLACK);
    t.fillCircle(cx+6 - shake*2, cy-4, 2, TFT_BLACK);
    t.drawLine(cx-4, cy+5, cx+4, cy+5, TFT_BLACK);

    // 锤子
    int hx = cx + r - 4, hy = cy - r + 2;
    t.fillRect(hx-1, hy, 2, 10, 0xBDD7);
    t.fillRect(hx-5, hy-5, 10, 6, 0xBDD7);
    return 150;
}

// ---- 回复: 开心跳 ----------------------------------------------
static uint32_t drawRespond(TFT_eSPI& t, int x, int y, int s, uint32_t ms) {
    int cx = x + s/2, cy = y + s/2 + 2, r = s/2 - 2;
    int bounce = ((ms / 150) % 2) ? 3 : 0;
    cy -= bounce;
    t.fillRect(x, y, s, s, Color::BG);
    drawFace(t, cx, cy, r, Color::SUCCESS);

    // 弯弯眼
    t.drawLine(cx-8, cy-5, cx-3, cy-2, TFT_BLACK);
    t.drawLine(cx-8, cy-4, cx-3, cy-1, TFT_BLACK);
    t.drawLine(cx+8, cy-5, cx+3, cy-2, TFT_BLACK);
    t.drawLine(cx+8, cy-4, cx+3, cy-1, TFT_BLACK);

    // 张嘴笑
    t.fillCircle(cx, cy + 4, 5, TFT_BLACK);
    t.fillCircle(cx, cy + 2, 5, Color::SUCCESS);

    // 星星
    if ((ms / 300) % 2) {
        int sx = cx + r - 8, sy = cy - r + 6;
        t.drawLine(sx-4, sy, sx+4, sy, TFT_YELLOW);
        t.drawLine(sx, sy-4, sx, sy+4, TFT_YELLOW);
        t.drawLine(sx-2, sy-2, sx+2, sy+2, TFT_YELLOW);
        t.drawLine(sx+2, sy-2, sx-2, sy+2, TFT_YELLOW);
    }
    return 200;
}

// ---- 主入口 ---------------------------------------------------
uint32_t Sprite::draw(TFT_eSPI& tft, int x, int y, int w, int h,
                       const char* status, uint32_t frameMs) {
    int s = min(w, h);

    if (strcmp(status, "thinking") == 0)
        return drawThink(tft, x, y, s, frameMs);
    else if (strcmp(status, "tool") == 0)
        return drawTool(tft, x, y, s, frameMs);
    else if (strcmp(status, "responding") == 0)
        return drawRespond(tft, x, y, s, frameMs);

    return drawIdle(tft, x, y, s, frameMs);
}
