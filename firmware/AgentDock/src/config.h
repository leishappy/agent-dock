// ============================================================
// AgentDock 全局配置
// ============================================================
#pragma once
#include <Arduino.h>

// --- 设备信息 -------------------------------------------------
#define DEVICE_NAME    "AgentDock"
#define FIRMWARE_VER   "0.2.0"

// --- WiFi -----------------------------------------------------
#define WIFI_AP_SSID     "AgentDock-Setup"
#define WIFI_AP_PASS     ""                  // 开放热点
#define WIFI_CONN_TIMEOUT  15000             // 连接超时 ms
#define MDNS_HOSTNAME    "agentdock"         // agentdock.local

// --- HTTP / WebSocket -----------------------------------------
#define HTTP_PORT         80
#define WS_PATH           "/ws"              // WebSocket 路径

// --- 显示 (ILI9341 240x320) -----------------------------------
#define SCREEN_WIDTH       240
#define SCREEN_HEIGHT      320
#define FRAME_INTERVAL_MS   50

// --- NVS 命名空间 ---------------------------------------------
#define NVS_WIFI_NS       "wifi"
#define NVS_KEY_SSID      "ssid"
#define NVS_KEY_PASS      "pass"

// --- 颜色主题 -------------------------------------------------
namespace Color {
    constexpr uint16_t BG       = 0x1082;
    constexpr uint16_t CARD     = 0x2104;
    constexpr uint16_t ACCENT   = 0x067F;
    constexpr uint16_t SUCCESS  = 0x06C4;
    constexpr uint16_t WARN     = 0xFEA0;
    constexpr uint16_t ERROR    = 0xF0C0;
    constexpr uint16_t TEXT     = 0xFFFF;
    constexpr uint16_t TEXT_DIM = 0x7BEF;
    constexpr uint16_t PROGRESS_BG = 0x39C7;
}
