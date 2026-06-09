// ============================================================
// ui.cpp — 单屏 UI + 像素角色动画
// ============================================================
#include "ui.h"
#include "display_mgr.h"
#include "sprite.h"

#define SPRITE_X 80
#define SPRITE_Y 28
#define SPRITE_S 80

UI& UI::instance() {
    static UI ui;
    return ui;
}

void UI::begin() {
    auto& tft = DisplayMgr::instance().tft();

    strncpy(_agent.status, "idle", sizeof(_agent.status)-1);
    strncpy(_agent.details, "等待连接...", sizeof(_agent.details)-1);

    tft.fillScreen(Color::BG);
    render();
    _dirty = false;
    _animStart = millis();
    Serial.println("[UI] 单屏模式就绪");
}

void UI::tick() {
    uint32_t now = millis();

    bool isActive = strcmp(_agent.status, "idle") != 0;
    bool animReady = (now - _lastFrame) > (_spriteInterval / 2);

    if (_dirty) {
        // 状态切换: 立即全屏重绘
        _lastFrame = now;
        render();
        _dirty = false;
        _animStart = now;
    } else if (isActive && animReady) {
        // 活跃: 高频刷新角色动画
        _lastFrame = now;
        renderSprite(now - _animStart);
    } else if (!isActive && animReady && (now - _lastFrame > 2000)) {
        // 空闲: 2s 眨一次
        _lastFrame = now;
        renderSprite(now - _animStart);
    }
}

// ---- 数据更新 -------------------------------------------------
void UI::updateTasks(const TaskInfo* tasks, uint8_t count) {
    count = min(count, (uint8_t)8);
    memcpy(_tasks, tasks, count * sizeof(TaskInfo));
    _taskCount = count;
    _dirty = true;
}

void UI::updateNotify(const NotifyInfo& n) {
    _notify = n;
    _dirty = true;
}

void UI::updateAgent(const AgentState& a) {
    // 只有状态变化才触发全屏重绘, 详情变化只更新文字不重绘角色区域
    bool changed = (strcmp(_agent.status, a.status) != 0);
    if (changed) {
        _animStart = millis();
        _dirty = true;
    }
    strncpy(_agent.status, a.status, sizeof(_agent.status)-1);
    strncpy(_agent.details, a.details, sizeof(_agent.details)-1);
}

void UI::updateIP(const char* ip) {
    strncpy(_ip, ip, sizeof(_ip)-1);
    _dirty = true;
}

// ---- 仅刷新角色区域 -------------------------------------------
void UI::renderSprite(uint32_t animMs) {
    auto& tft = DisplayMgr::instance().tft();
    Sprite::draw(tft, SPRITE_X, SPRITE_Y, SPRITE_S, SPRITE_S, _agent.status, animMs);
}

// ============================================================
// 全屏渲染
// ============================================================
void UI::render() {
    auto& tft = DisplayMgr::instance().tft();
    tft.fillScreen(Color::BG);

    int y = 0;

    // ===== 顶栏 =====
    tft.fillRect(0, y, SCREEN_WIDTH, 26, Color::CARD);
    tft.setTextColor(Color::ACCENT, Color::CARD);
    tft.setTextFont(2);
    tft.setCursor(6, y + 7);
    tft.print("AgentDock");
    tft.setTextColor(Color::TEXT_DIM, Color::CARD);
    tft.setCursor(SCREEN_WIDTH - 100, y + 7);
    tft.print(_ip);
    y += 26;

    // ===== 角色动画 =====
    uint32_t animMs = millis() - _animStart;
    Sprite::draw(tft, SPRITE_X, SPRITE_Y, SPRITE_S, SPRITE_S, _agent.status, animMs);

    // Agent 状态文字 (角色下方)
    uint16_t dotC = Color::TEXT_DIM;
    const char* statusText = "Idle";
    if      (strcmp(_agent.status, "thinking") == 0)   { dotC = Color::ACCENT;  statusText = "Thinking..."; }
    else if (strcmp(_agent.status, "tool") == 0)       { dotC = Color::WARN;    statusText = "Working"; }
    else if (strcmp(_agent.status, "responding") == 0)  { dotC = Color::SUCCESS; statusText = "Responding"; }

    tft.setTextColor(dotC, Color::BG);
    tft.setTextFont(2);
    tft.setCursor((SCREEN_WIDTH - 80) / 2, SPRITE_Y + SPRITE_S + 2);
    tft.print(statusText);

    if (_agent.details[0]) {
        tft.setTextColor(Color::TEXT_DIM, Color::BG);
        tft.setCursor((SCREEN_WIDTH - 160) / 2, SPRITE_Y + SPRITE_S + 18);
        char buf[28];
        strncpy(buf, _agent.details, sizeof(buf)-1);
        buf[sizeof(buf)-1] = 0;
        tft.print(buf);
    }

    y = SPRITE_Y + SPRITE_S + 36;

    tft.drawLine(4, y, SCREEN_WIDTH - 4, y, Color::CARD);
    y += 3;

    // ===== 任务列表 =====
    int taskH = 26;
    int maxTasks = min((int)_taskCount, 3);
    for (int i = 0; i < maxTasks; i++) {
        auto& tk = _tasks[i];

        tft.fillRect(4, y, SCREEN_WIDTH - 8, taskH - 2, Color::CARD);

        uint16_t barC = Color::ACCENT;
        if (tk.progress >= 100 || strcmp(tk.status, "done") == 0) barC = Color::SUCCESS;
        else if (strcmp(tk.status, "error") == 0) barC = Color::ERROR;

        tft.setTextColor(Color::TEXT, Color::CARD);
        tft.setTextFont(2);
        tft.setCursor(10, y + 4);
        char name[24];
        strncpy(name, tk.name, sizeof(name)-1);
        name[sizeof(name)-1] = 0;
        tft.print(name);

        tft.setTextColor(Color::TEXT_DIM, Color::CARD);
        tft.setCursor(SCREEN_WIDTH - 43, y + 4);
        tft.print(tk.progress);
        tft.print("%");

        // 细进度条
        tft.fillRect(10, y + taskH - 6, SCREEN_WIDTH - 20, 3, Color::PROGRESS_BG);
        int barW = tk.progress * (SCREEN_WIDTH - 20) / 100;
        if (barW > 0) tft.fillRect(10, y + taskH - 6, barW, 3, barC);

        y += taskH;
    }

    if (_taskCount == 0) {
        tft.setTextColor(Color::TEXT_DIM, Color::BG);
        tft.setTextFont(2);
        tft.setCursor(70, y + 16);
        tft.print("No tasks");
        y += 38;
    }

    tft.drawLine(4, y, SCREEN_WIDTH - 4, y, Color::CARD);
    y += 3;

    // ===== 通知 =====
    uint16_t lvlC = Color::TEXT_DIM;
    const char* lvlIcon = "i";
    if      (strcmp(_notify.level, "warn") == 0)  { lvlC = Color::WARN;  lvlIcon = "!"; }
    else if (strcmp(_notify.level, "error") == 0) { lvlC = Color::ERROR; lvlIcon = "X"; }
    else if (strcmp(_notify.level, "info") == 0)  { lvlC = Color::ACCENT; lvlIcon = "i"; }

    tft.fillCircle(14, y + 12, 8, lvlC);
    tft.setTextColor(Color::TEXT, lvlC);
    tft.setTextFont(4);
    tft.setCursor(11, y + 4);
    tft.print(lvlIcon);

    tft.setTextColor(Color::TEXT, Color::BG);
    tft.setTextFont(2);
    tft.setCursor(30, y + 4);
    char title[28];
    strncpy(title, _notify.title, sizeof(title)-1);
    title[sizeof(title)-1] = 0;
    if (title[0] && strcmp(title, "---") != 0) {
        tft.print(title);
        tft.setTextColor(Color::TEXT_DIM, Color::BG);
        tft.setCursor(30, y + 18);
        char body[36];
        strncpy(body, _notify.body, sizeof(body)-1);
        body[sizeof(body)-1] = 0;
        tft.print(body);
    } else {
        tft.print("No notices");
    }
}
