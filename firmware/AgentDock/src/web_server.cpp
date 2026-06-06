// ============================================================
// web_server.cpp
// ============================================================
#include <Preferences.h>
#include <ArduinoJson.h>
#include "web_server.h"
#include "wifi_mgr.h"

// --- 控制面板 HTML ---------------------------------------------
static const char* PANEL_HTML = R"raw(
<!DOCTYPE html>
<html lang="zh">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>AgentDock</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,sans-serif;background:#0f1923;color:#e0e0e0;padding:12px}
.card{background:#1a2733;border-radius:14px;padding:20px;max-width:440px;margin:auto;
      box-shadow:0 8px 32px rgba(0,0,0,.5)}
h2{font-size:17px;margin-bottom:14px;color:#26a69a}
.tabs{display:flex;gap:4px;margin-bottom:16px}
.tab{padding:8px 16px;border-radius:8px 8px 0 0;background:#1e2e3a;color:#8899aa;
     font-size:13px;cursor:pointer;border:none}
.tab.active{background:#26a69a;color:#fff}
.panel{display:none}.panel.active{display:block}
label{display:block;font-size:12px;color:#8899aa;margin:10px 0 3px}
input,select,textarea{width:100%;padding:10px;border-radius:8px;border:1px solid #2a3a46;
       background:#0d1a24;color:#fff;font-size:14px;outline:none;resize:vertical}
input:focus,select:focus,textarea:focus{border-color:#26a69a}
.row{display:flex;justify-content:space-between;padding:10px 0;border-bottom:1px solid #1e2e3a}
.row .l{color:#8899aa;font-size:13px}.row .v{color:#fff;font-size:13px}
.btn{width:100%;padding:12px;margin-top:10px;border:none;border-radius:8px;font-size:14px;cursor:pointer}
.btn-go{background:#26a69a;color:#fff}
.btn-red{background:#c0392b;color:#fff}
.btn-blue{background:#2980b9;color:#fff}
.status{font-size:12px;color:#5a7a8a;margin-top:6px;min-height:18px}
.dot{display:inline-block;width:8px;height:8px;border-radius:50%;margin-right:6px}
.g{background:#27ae60}.r{background:#e74c3c}
.flex2{display:flex;gap:8px}.flex2>*{flex:1}
</style>
</head>
<body>
<div class="card">
  <h2>⚓ AgentDock</h2>
  <div class="tabs">
    <button class="tab active" onclick="showTab(0)">Status</button>
    <button class="tab" onclick="showTab(1)">Test</button>
    <button class="tab" onclick="showTab(2)">Config</button>
  </div>

  <!-- Status -->
  <div class="panel active">
    <div class="row"><span class="l">WiFi</span><span class="v"><span class="dot g"></span>Connected</span></div>
    <div class="row"><span class="l">IP</span><span class="v" id="ip">-</span></div>
    <div class="row"><span class="l">WebSocket</span><span class="v" id="ws">-</span></div>
    <div class="row"><span class="l">Heap Free</span><span class="v" id="heap">-</span></div>
    <div class="row"><span class="l">Version</span><span class="v" id="ver">-</span></div>
    <button class="btn btn-red" onclick="forget()" style="margin-top:20px">Forget WiFi & Reset</button>
  </div>

  <!-- Test -->
  <div class="panel">
    <label>Tasks (JSON)</label>
    <textarea id="task_json" rows="5">[
  {"id":"1","name":"Compile firmware","pct":85,"status":"running"},
  {"id":"2","name":"Run tests","pct":60,"status":"running"},
  {"id":"3","name":"Review PR","pct":30,"status":"pending"}
]</textarea>
    <button class="btn btn-go" onclick="sendTasks()">Send Tasks</button>

    <label>City</label>
    <div class="flex2">
      <input id="w_city" value="Shenzhen" placeholder="City">
      <select id="w_code"><option>sunny</option><option>cloudy</option><option>rain</option></select>
    </div>
    <div class="flex2">
      <input id="w_temp" type="number" value="26" placeholder="Temp">
      <input id="w_hum" type="number" value="65" placeholder="Humidity %">
    </div>
    <button class="btn btn-go" onclick="sendWeather()">Send Weather</button>

    <label>Notification</label>
    <input id="n_title" value="Build Complete" placeholder="Title">
    <input id="n_body" value="All tests passed" placeholder="Body">
    <select id="n_level"><option>info</option><option>warn</option><option>error</option></select>
    <button class="btn btn-go" onclick="sendNotify()">Send Notification</button>

    <div class="status" id="test_status"></div>
  </div>

  <!-- Config -->
  <div class="panel">
    <label>Weather City</label>
    <input id="cfg_city" value="Shenzhen" placeholder="City name">
    <label>Screen Cycle (seconds)</label>
    <input id="cfg_cycle" type="number" value="5" placeholder="Seconds per screen">
    <button class="btn btn-go" onclick="saveConfig()">Save Config</button>
    <div class="status" id="cfg_status"></div>
  </div>
</div>
<script>
function showTab(n){
  document.querySelectorAll('.tab').forEach((e,i)=>e.classList.toggle('active',i===n));
  document.querySelectorAll('.panel').forEach((e,i)=>e.classList.toggle('active',i===n))}

async function api(method,path,body){
  let opts={method};
  if(body){opts.headers={'Content-Type':'application/json'};opts.body=JSON.stringify(body)}
  return fetch(path,opts).then(r=>r.json())}

async function load(){
  try{let j=await fetch('/status').then(r=>r.json());
      document.getElementById('ver').textContent=j.ver;
      document.getElementById('ip').textContent=j.ip;
      document.getElementById('ws').textContent=j.ws_clients;
      document.getElementById('heap').textContent=(j.heap/1024).toFixed(1)+' KB'}
  catch(e){}}

async function sendTasks(){
  let s=document.getElementById('test_status');
  try{let d=JSON.parse(document.getElementById('task_json').value);
      await api('POST','/api/tasks',{list:d});s.textContent='Tasks sent!'}
  catch(e){s.textContent='Error: '+e.message}}

async function sendWeather(){
  let s=document.getElementById('test_status');
  try{await api('POST','/api/weather',{
      city:document.getElementById('w_city').value,
      temp:parseInt(document.getElementById('w_temp').value),
      humidity:parseInt(document.getElementById('w_hum').value),
      code:document.getElementById('w_code').value});
      s.textContent='Weather sent!'}
  catch(e){s.textContent='Error: '+e.message}}

async function sendNotify(){
  let s=document.getElementById('test_status');
  try{await api('POST','/api/notify',{
      title:document.getElementById('n_title').value,
      body:document.getElementById('n_body').value,
      level:document.getElementById('n_level').value});
      s.textContent='Notification sent!'}
  catch(e){s.textContent='Error: '+e.message}}

async function saveConfig(){
  let s=document.getElementById('cfg_status');
  try{await api('POST','/api/config',{
      city:document.getElementById('cfg_city').value,
      cycle:parseInt(document.getElementById('cfg_cycle').value)});
      s.textContent='Saved!'}
  catch(e){s.textContent='Error: '+e.message}}

async function forget(){
  if(confirm('Reset WiFi settings?')){await fetch('/forget');location.reload()}}

load();setInterval(load,10000);
</script>
</body>
</html>
)raw";

WebServer& WebServer::instance() {
    static WebServer server;
    return server;
}

void WebServer::begin() {
    _http = new AsyncWebServer(HTTP_PORT);
    _ws   = new AsyncWebSocket(WS_PATH);

    setupCommonRoutes();

    if (WifiMgr::instance().state() == WifiState::AP_MODE) {
        setupAPRoutes();
    } else {
        setupNormalRoutes();
    }

    _ws->onEvent(onWsEvent);
    _http->addHandler(_ws);
    _http->begin();

    Serial.printf("[Web] HTTP + WS 启动, 端口 %d\n", HTTP_PORT);
}

void WebServer::tick() {
    if (_ws) _ws->cleanupClients();
}

// --- 通用路由 -------------------------------------------------
void WebServer::setupCommonRoutes() {
    // 设备状态
    _http->on("/status", HTTP_GET, [](AsyncWebServerRequest* req) {
        AsyncResponseStream* r = req->beginResponseStream("application/json");
        r->printf("{\"ver\":\"%s\",\"ip\":\"%s\",\"ws_clients\":%d,\"heap\":%d}",
                  FIRMWARE_VER, WiFi.localIP().toString().c_str(),
                  WebServer::instance()._ws ? WebServer::instance()._ws->count() : 0,
                  ESP.getFreeHeap());
        req->send(r);
    });

    // 清除 WiFi 并重启
    _http->on("/forget", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "text/plain", "OK, rebooting...");
        delay(200);
        WifiMgr::instance().forget();
    });
}

// --- AP 模式路由 ----------------------------------------------
void WebServer::setupAPRoutes() {
    // 配网页
    _http->on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "text/html", WifiMgr::CAPTIVE_HTML);
    });

    // WiFi 扫描
    _http->on("/scan", HTTP_GET, [](AsyncWebServerRequest* req) {
        int n = WiFi.scanNetworks();
        AsyncResponseStream* r = req->beginResponseStream("application/json");
        r->print("[");
        for (int i = 0; i < n; i++) {
            if (i > 0) r->print(",");
            r->printf("\"%s\"", WiFi.SSID(i).c_str());
        }
        r->print("]");
        req->send(r);
        WiFi.scanDelete();
    });

    // 连接提交
    _http->on("/connect", HTTP_GET, [](AsyncWebServerRequest* req) {
        String ssid = req->getParam("ssid") ? req->getParam("ssid")->value() : "";
        String pass = req->getParam("pass") ? req->getParam("pass")->value() : "";

        if (ssid.length() == 0) {
            req->send(400, "application/json", "{\"ok\":false,\"msg\":\"SSID required\"}");
            return;
        }

        req->send(200, "application/json", "{\"ok\":true}");
        WifiMgr::instance().saveAndReboot(ssid.c_str(), pass.c_str());
    });

    // Captive Portal 兜底：所有未匹配请求 → 配网页
    _http->onNotFound([](AsyncWebServerRequest* req) {
        req->redirect("/");
    });
}

// --- 通用: body 解析 + 注入 type 字段 → 协议处理器 --------------
static void handleApi(const char* type, AsyncWebServerRequest* req,
                      uint8_t* data, size_t len) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (err) {
        req->send(400, "application/json", "{\"ok\":false,\"msg\":\"bad JSON\"}");
        return;
    }
    doc["t"] = type;
    String out;
    serializeJson(doc, out);
    WsProto::instance().handleMessage(out.c_str(), out.length());
    req->send(200, "application/json", "{\"ok\":true}");
}

// --- 正常模式路由 ---------------------------------------------
void WebServer::setupNormalRoutes() {
    // 控制面板
    _http->on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "text/html", PANEL_HTML);
    });

    // ---- API: 任务 ----
    _http->on("/api/tasks", HTTP_POST,
              [](AsyncWebServerRequest* req) {},
              nullptr,
              [](AsyncWebServerRequest* req, uint8_t* d, size_t l, size_t, size_t) {
                  handleApi("tasks", req, d, l);
              });

    // ---- API: 天气 ----
    _http->on("/api/weather", HTTP_POST,
              [](AsyncWebServerRequest* req) {},
              nullptr,
              [](AsyncWebServerRequest* req, uint8_t* d, size_t l, size_t, size_t) {
                  handleApi("weather", req, d, l);
              });

    // ---- API: 通知 ----
    _http->on("/api/notify", HTTP_POST,
              [](AsyncWebServerRequest* req) {},
              nullptr,
              [](AsyncWebServerRequest* req, uint8_t* d, size_t l, size_t, size_t) {
                  handleApi("notify", req, d, l);
              });

    // ---- API: 配置 ----
    _http->on("/api/config", HTTP_POST,
              [](AsyncWebServerRequest* req) {},
              nullptr,
              [](AsyncWebServerRequest* req, uint8_t* d, size_t l, size_t, size_t) {
                  JsonDocument doc;
                  deserializeJson(doc, d, l);
                  Preferences prefs;
                  prefs.begin("config", false);
                  if (doc.containsKey("city"))
                      prefs.putString("city", doc["city"] | "Shenzhen");
                  prefs.end();
                  req->send(200, "application/json", "{\"ok\":true}");
              });

    _http->onNotFound([](AsyncWebServerRequest* req) {
        req->send(404, "text/plain", "Not Found");
    });
}

// --- WebSocket 事件 -------------------------------------------
void WebServer::onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                          AwsEventType type, void* arg, uint8_t* data, size_t len) {
    auto& proto = WsProto::instance();

    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("[WS] 客户端连接 #%u, 来自 %s\n",
                          client->id(), client->remoteIP().toString().c_str());
            // 注入发送函数
            proto.setSendFn([client](const char* msg) {
                if (client && client->status() == WS_CONNECTED) {
                    client->text(msg);
                }
            });
            proto.sendHello();
            proto.handleConnect();
            break;

        case WS_EVT_DISCONNECT:
            Serial.printf("[WS] 客户端断开 #%u\n", client->id());
            break;

        case WS_EVT_DATA: {
            AwsFrameInfo* info = (AwsFrameInfo*)arg;
            if (info->final && info->opcode == WS_TEXT) {
                data[len] = 0;  // null terminate
                proto.handleMessage((const char*)data, len);
            }
            break;
        }

        default:
            break;
    }
}
