// ============================================================
// web_server.h — HTTP + WebSocket 服务器
// ============================================================
// HTTP:
//   GET  /          → 配网页面 (AP模式) / 控制面板 (已连接)
//   GET  /scan      → WiFi 扫描结果 JSON
//   GET  /connect   → 接收 ssid & pass，保存并重启
//   GET  /status    → 设备状态 JSON
//
// WebSocket:
//   ws://<ip>/ws    → 双向 JSON 消息
// ============================================================
#pragma once
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "config.h"
#include "ws_proto.h"

class WebServer {
public:
    static WebServer& instance();

    void begin();     // 启动 HTTP + WebSocket
    void tick();      // 每帧清理断线

private:
    WebServer() = default;

    AsyncWebServer*  _http = nullptr;
    AsyncWebSocket*  _ws   = nullptr;

    // WebSocket 事件
    static void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                          AwsEventType type, void* arg, uint8_t* data, size_t len);

    // HTTP 路由注册
    void setupAPRoutes();
    void setupNormalRoutes();
    void setupCommonRoutes();
};
