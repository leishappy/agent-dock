// ============================================================
// AgentDock — 主入口 (WiFi + WebSocket 版)
// ============================================================
#include <Arduino.h>
#include <ESPmDNS.h>
#include "config.h"
#include "wifi_mgr.h"
#include "web_server.h"
#include "ws_proto.h"
#include "display_mgr.h"
#include "ui.h"

auto& wifi   = WifiMgr::instance();
auto& web    = WebServer::instance();
auto& proto  = WsProto::instance();
auto& disp   = DisplayMgr::instance();
auto& ui     = UI::instance();

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

    // 1. 显示初始化
    disp.begin();
    ui.begin();

    // 2. WiFi
    bool connected = wifi.begin();

    if (connected) {
        Serial.printf("[Main] WiFi 已连接: %s\n", wifi.ip());
        ui.updateIP(wifi.ip());
        ledBlink(3, 150);

        if (MDNS.begin(MDNS_HOSTNAME)) {
            MDNS.addService("http", "tcp", HTTP_PORT);
            MDNS.addServiceTxt("http", "tcp", "dev", DEVICE_NAME);
            Serial.printf("[Main] mDNS: %s.local\n", MDNS_HOSTNAME);
        } else {
            Serial.println("[Main] mDNS 启动失败");
        }
    } else {
        Serial.printf("[Main] AP 配网模式: %s\n", wifi.ip());
        ui.updateIP(wifi.ip());
        ledBlink(1, 800);
    }

    // 3. Web 服务器
    web.begin();

    // 4. 注册业务回调 → UI
    proto.onConnect([]() {
        Serial.println("[Main] WS 客户端已连接");
        proto.sendReady();
    });

    proto.onTasks([](const TaskInfo* tasks, uint8_t count) {
        Serial.printf("[Main] 收到 %d 个任务\n", count);
        ui.updateTasks(tasks, count);
    });

    proto.onNotify([](const NotifyInfo& n) {
        Serial.printf("[Main] 通知 [%s]: %s\n", n.level, n.title);
        ui.updateNotify(n);
    });

    proto.onAgent([](const AgentState& a) {
        Serial.printf("[Main] Agent %s: %s\n", a.status, a.details);
        ui.updateAgent(a);
    });

    Serial.println("[Main] 启动完成");
}

void loop() {
    wifi.tick();
    web.tick();
    ui.tick();
    delay(20);
}
