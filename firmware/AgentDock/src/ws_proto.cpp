// ============================================================
// ws_proto.cpp
// ============================================================
#include <WiFi.h>
#include "ws_proto.h"

WsProto& WsProto::instance() {
    static WsProto proto;
    return proto;
}

// --- WebSocket 消息入口 ---------------------------------------
void WsProto::handleMessage(const char* data, size_t len) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err) {
        sendErr("JSON parse error");
        return;
    }

    const char* type = doc["t"];
    if (!type) { sendErr("missing 't'"); return; }

    // ---- tasks -------------------------------------------------
    if (strcmp(type, "tasks") == 0) {
        JsonArray arr = doc["list"].as<JsonArray>();
        if (arr.size() == 0) return;

        TaskInfo tasks[8];
        uint8_t   count = 0;
        for (JsonVariant item : arr) {
            if (count >= 8) break;
            auto& t = tasks[count];
            strncpy(t.id,      item["id"]     | "", sizeof(t.id)-1);
            strncpy(t.name,    item["name"]   | "", sizeof(t.name)-1);
            strncpy(t.status,  item["status"] | "pending", sizeof(t.status)-1);
            t.progress = item["pct"] | 0;
            count++;
        }
        if (_onTasks) _onTasks(tasks, count);

    // ---- weather ------------------------------------------------
    } else if (strcmp(type, "weather") == 0) {
        WeatherInfo w;
        w.temp     = doc["temp"]     | 0;
        w.humidity = doc["humidity"] | 0;
        strncpy(w.code, doc["code"] | "unknown", sizeof(w.code)-1);
        strncpy(w.city, doc["city"] | "",       sizeof(w.city)-1);
        if (_onWeather) _onWeather(w);

    // ---- notify -------------------------------------------------
    } else if (strcmp(type, "notify") == 0) {
        NotifyInfo n;
        strncpy(n.title, doc["title"] | "",       sizeof(n.title)-1);
        strncpy(n.body,  doc["body"]  | "",       sizeof(n.body)-1);
        strncpy(n.level, doc["level"] | "info",   sizeof(n.level)-1);
        if (_onNotify) _onNotify(n);

    // ---- agent (Claude Code 状态) -----------------------------------
    } else if (strcmp(type, "agent") == 0) {
        AgentState a;
        strncpy(a.status,  doc["status"]  | "idle", sizeof(a.status)-1);
        strncpy(a.details, doc["details"] | "",    sizeof(a.details)-1);
        a.ts    = doc["ts"] | 0;
        if (_onAgent) _onAgent(a);

    // ---- screen -------------------------------------------------
    } else if (strcmp(type, "screen") == 0) {
        uint8_t s = doc["id"] | 0;
        if (_onScreen) _onScreen(s);

    // ---- ping (心跳) --------------------------------------------
    } else if (strcmp(type, "ping") == 0) {
        Serial.println("[Proto] ping -> pong");
        sendPong();

    } else {
        sendErr("unknown command");
    }
}

// --- 连接事件 -------------------------------------------------
void WsProto::handleConnect() {
    if (_onConnect) _onConnect();
}

// --- 快捷发送 -------------------------------------------------
void WsProto::sendHello() {
    if (!_send) return;
    JsonDocument doc;
    doc["t"]   = "hello";
    doc["dev"] = DEVICE_NAME;
    doc["ver"] = FIRMWARE_VER;
    String buf;
    serializeJson(doc, buf);
    _send(buf.c_str());
}

void WsProto::sendReady() {
    if (!_send) return;
    JsonDocument doc;
    doc["t"]  = "ready";
    doc["ip"] = WiFi.localIP().toString();
    String buf;
    serializeJson(doc, buf);
    _send(buf.c_str());
}

void WsProto::sendPong() {
    if (_send) _send("{\"t\":\"pong\"}");
}

void WsProto::sendErr(const char* msg) {
    if (!_send) return;
    JsonDocument doc;
    doc["t"]   = "err";
    doc["msg"] = msg;
    String buf;
    serializeJson(doc, buf);
    _send(buf.c_str());
}
