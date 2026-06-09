// ============================================================
// ui.h — 单屏 UI + 像素角色动画
// ============================================================
// 布局 (240x320):
//   顶栏:  设备名 + IP
//   角色:  80x80 像素动画 (idle/think/tool/respond)
//   任务:  卡片列表 + 进度条
//   通知:  最新一条
// ============================================================
#pragma once
#include <TFT_eSPI.h>
#include "config.h"
#include "ws_proto.h"

class UI {
public:
    static UI& instance();

    void begin();
    void tick();

    void updateTasks(const TaskInfo* tasks, uint8_t count);
    void updateNotify(const NotifyInfo& n);
    void updateAgent(const AgentState& a);
    void updateIP(const char* ip);

private:
    void render();
    void renderSprite(uint32_t animMs);  // 仅重绘角色区域

    TaskInfo    _tasks[8];
    uint8_t     _taskCount = 0;
    NotifyInfo  _notify;
    AgentState  _agent;
    char        _ip[16] = "---.---.---.---";

    bool     _dirty = true;
    uint32_t _lastFrame  = 0;
    uint32_t _animStart  = 0;  // 当前动画状态的起始时间
    uint32_t _spriteInterval = 200;
};
