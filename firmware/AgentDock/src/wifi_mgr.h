// ============================================================
// wifi_mgr.h — WiFi 管理器
// ============================================================
// 连接流程:
//   1. 检查 NVS 中是否有保存的 WiFi 凭据
//   2. 有 → 连接 → 成功 → 返回 true
//   3. 无/失败 → 启动 AP 热点 + Captive DNS + 配网页
//   4. 用户提交 WiFi → 保存 NVS → 重启
// ============================================================
#pragma once
#include <WiFi.h>
#include <DNSServer.h>
#include <Preferences.h>
#include "config.h"

enum class WifiState {
    CONNECTING,     // 正在连接
    CONNECTED,      // 已连接
    AP_MODE,        // 配网模式（热点）
    DISCONNECTED    // 断开
};

class WifiMgr {
public:
    static WifiMgr& instance();

    // 返回 true = WiFi 已连接, false = 进入 AP 配网模式
    bool begin();

    // 每帧调用，处理配网模式的 DNS + HTTP 请求
    void tick();

    WifiState state() const { return _state; }
    const char* ip()    const { return _ip; }

    // 保存凭据并重启（供 WebServer 调用）
    bool saveAndReboot(const char* ssid, const char* pass);

    // 清除保存的凭据
    void forget();

    // 配网页（HTML 内嵌）
    static const char* CAPTIVE_HTML;

private:
    WifiMgr() = default;

    bool connectSaved();
    void startAP();

    WifiState  _state = WifiState::DISCONNECTED;
    char       _ip[16] = {0};
    uint32_t   _connStart = 0;

    // AP 模式专用
    DNSServer* _dns = nullptr;
    bool       _apMode = false;
};
