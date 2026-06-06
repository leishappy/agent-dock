// ============================================================
// ws_proto.h — WebSocket 通信协议
// ============================================================
// JSON 格式 (同 USB CDC 协议, 改走 WebSocket):
//   PC → ESP:  {"t":"tasks","list":[...]}
//   PC → ESP:  {"t":"weather","temp":26,"code":"sunny"}
//   PC → ESP:  {"t":"notify","title":"...","body":"...","level":"info"}
//   PC → ESP:  {"t":"screen","id":0}
//   PC → ESP:  {"t":"ping"}   →  ESP → PC:  {"t":"pong"}
//
//   ESP → PC:  {"t":"ready","ip":"192.168.1.100"}
//   ESP → PC:  {"t":"hello","dev":"AgentDock","ver":"0.2.0"}
// ============================================================
#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>
#include "config.h"

// --- 数据结构 -------------------------------------------------
struct TaskInfo {
    char     id[16];
    char     name[64];
    uint8_t  progress;
    char     status[16];
};

struct WeatherInfo {
    int16_t  temp;
    uint8_t  humidity;
    char     code[16];
    char     city[32];
};

struct NotifyInfo {
    char     title[64];
    char     body[128];
    char     level[12];
};

// --- Agent 状态 ------------------------------------------------
struct AgentState {
    char     status[16];    // "idle" / "thinking" / "tool" / "responding"
    char     details[64];
    uint32_t ts;
};

// --- 回调类型 -------------------------------------------------
using TaskCallback    = std::function<void(const TaskInfo* tasks, uint8_t count)>;
using WeatherCallback = std::function<void(const WeatherInfo&)>;
using NotifyCallback  = std::function<void(const NotifyInfo&)>;
using AgentCallback   = std::function<void(const AgentState&)>;
using ScreenCallback  = std::function<void(uint8_t screen)>;
using ConnectCallback = std::function<void()>;

// --- 协议处理器 -----------------------------------------------
class WsProto {
public:
    static WsProto& instance();

    // 注册业务回调
    void onTasks(TaskCallback cb)       { _onTasks = cb; }
    void onWeather(WeatherCallback cb)  { _onWeather = cb; }
    void onNotify(NotifyCallback cb)    { _onNotify = cb; }
    void onAgent(AgentCallback cb)      { _onAgent = cb; }
    void onScreen(ScreenCallback cb)    { _onScreen = cb; }
    void onConnect(ConnectCallback cb)  { _onConnect = cb; }

    // WebSocket 事件入口（由 WebServer 调用）
    void handleMessage(const char* data, size_t len);
    void handleConnect();     // 客户端连接时调用

    // 发送（需要 WebServer 回调注入）
    using SendFn = std::function<void(const char* json)>;
    void setSendFn(SendFn fn) { _send = fn; }

    // 快捷发送
    void sendHello();
    void sendReady();
    void sendPong();
    void sendErr(const char* msg);

private:
    WsProto() = default;

    TaskCallback    _onTasks    = nullptr;
    WeatherCallback _onWeather  = nullptr;
    NotifyCallback  _onNotify   = nullptr;
    AgentCallback   _onAgent    = nullptr;
    ScreenCallback  _onScreen   = nullptr;
    ConnectCallback _onConnect  = nullptr;
    SendFn          _send       = nullptr;
};
