// ============================================================
// AgentDock — 主入口 (WiFi + WebSocket 版)
// ============================================================
// 启动流程:
//   1. WiFi 连接（已存凭据）或 进入 AP 配网模式
//   2. 启动 HTTP + WebSocket 服务器
//   3. 启动 mDNS (agentdock.local)
//   4. 主循环：WiFi管维护 + WS 客户端清理
//
// 配网:
//   手机连接 "AgentDock-Setup" 热点 → 自动弹出配网页
//   → 选 WiFi 输密码 → 保存 → 自动重启 → 连接
//
// 正常模式:
//   浏览器 http://agentdock.local → 控制面板
//   WebSocket ws://agentdock.local/ws → 数据通道
// ============================================================
#include <Arduino.h>
#include <ESPmDNS.h>
#include "config.h"
#include "wifi_mgr.h"
#include "web_server.h"
#include "ws_proto.h"

auto& wifi   = WifiMgr::instance();
auto& web    = WebServer::instance();
auto& proto  = WsProto::instance();

// --- 状态指示灯 -----------------------------------------------
static void ledBlink(int times, int ms = 200) {
    pinMode(LED_BUILTIN, OUTPUT);
    for (int i = 0; i < times; i++) {
        digitalWrite(LED_BUILTIN, HIGH); delay(ms);
        digitalWrite(LED_BUILTIN, LOW);  delay(ms);
    }
}

// ============================================================
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.printf("\n=== %s v%s ===\n", DEVICE_NAME, FIRMWARE_VER);

    // 1. WiFi
    bool connected = wifi.begin();

    if (connected) {
        // 正常模式
        Serial.printf("[Main] WiFi 已连接: %s\n", wifi.ip());

        // 指示灯：快闪 3 下 = 连接成功
        ledBlink(3, 150);

        // mDNS
        if (MDNS.begin(MDNS_HOSTNAME)) {
            MDNS.addService("http", "tcp", HTTP_PORT);
            MDNS.addServiceTxt("http", "tcp", "dev", DEVICE_NAME);
            Serial.printf("[Main] mDNS: %s.local\n", MDNS_HOSTNAME);
        } else {
            Serial.println("[Main] mDNS 启动失败");
        }
    } else {
        // AP 配网模式
        Serial.printf("[Main] AP 配网模式: %s\n", wifi.ip());

        // 指示灯：慢闪 = 配网模式
        ledBlink(1, 800);
    }

    // 2. Web 服务器
    web.begin();

    // 3. 注册业务回调（UI 后续整合）
    proto.onConnect([]() {
        Serial.println("[Main] WS 客户端已连接，发送 ready");
        proto.sendReady();
    });

    proto.onTasks([](const TaskInfo* tasks, uint8_t count) {
        Serial.printf("[Main] 收到 %d 个任务\n", count);
        for (uint8_t i = 0; i < count; i++) {
            Serial.printf("  [%s] %s (%d%%)\n",
                          tasks[i].id, tasks[i].name, tasks[i].progress);
        }
    });

    proto.onWeather([](const WeatherInfo& w) {
        Serial.printf("[Main] 天气: %s %d°C, %d%%\n", w.city, w.temp, w.humidity);
    });

    proto.onNotify([](const NotifyInfo& n) {
        Serial.printf("[Main] 通知 [%s]: %s\n", n.level, n.title);
    });

    proto.onAgent([](const AgentState& a) {
        const char* emoji = "?";
        if (strcmp(a.status, "idle") == 0)       emoji = "[IDLE]";
        else if (strcmp(a.status, "thinking") == 0)  emoji = "[THINK]";
        else if (strcmp(a.status, "tool") == 0)      emoji = "[TOOL]";
        else if (strcmp(a.status, "responding") == 0) emoji = "[RESP]";
        Serial.printf("[Main] Agent %s %s\n", emoji, a.details);
    });

    proto.onScreen([](uint8_t s) {
        Serial.printf("[Main] 切屏: %d\n", s);
    });

    Serial.println("[Main] 启动完成");
}

// ============================================================
void loop() {
    wifi.tick();
    web.tick();
    delay(20);
}
