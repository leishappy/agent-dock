// ============================================================
// wifi_mgr.cpp
// ============================================================
#include "wifi_mgr.h"

// --- 配网页面 HTML --------------------------------------------
const char* WifiMgr::CAPTIVE_HTML = R"raw(
<!DOCTYPE html>
<html lang="zh">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>AgentDock Setup</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,sans-serif;background:#0f1923;color:#e0e0e0;
     display:flex;justify-content:center;align-items:center;min-height:100vh;padding:20px}
.card{background:#1a2733;border-radius:16px;padding:32px 24px;max-width:360px;width:100%;
      box-shadow:0 8px 32px rgba(0,0,0,.5);text-align:center}
h1{font-size:22px;margin-bottom:4px}.ver{font-size:12px;color:#5a7a8a;margin-bottom:24px}
label{display:block;text-align:left;font-size:13px;color:#8899aa;margin:12px 0 4px}
select,input{width:100%;padding:12px;border-radius:10px;border:1px solid #2a3a46;
             background:#0d1a24;color:#fff;font-size:15px;outline:none}
input:focus,select:focus{border-color:#26a69a}
button{width:100%;padding:14px;margin-top:20px;border:none;border-radius:10px;
       background:#26a69a;color:#fff;font-size:16px;font-weight:600;cursor:pointer}
button:active{background:#1b7a72}
.status{font-size:12px;color:#5a7a8a;margin-top:8px}
</style>
</head>
<body>
<div class="card">
  <h1>⚓ AgentDock</h1>
  <div class="ver">First-time Setup</div>
  <form onsubmit="connect(event)">
    <label>WiFi Network</label>
    <select id="ssid_list" onchange="fillSSID()"></select>
    <input id="ssid" type="text" placeholder="Or type SSID..." autocomplete="off" required>
    <label>Password</label>
    <input id="pass" type="password" placeholder="WiFi password (min 8 chars)">
    <button type="submit">Connect</button>
    <div class="status" id="status"></div>
  </form>
</div>
<script>
async function scan(){
  try{let r=await fetch('/scan');let list=await r.json();
      let sel=document.getElementById('ssid_list');
      list.forEach(s=>{let o=document.createElement('option');o.value=s;o.text=s;sel.add(o)})}
  catch(e){}
}
function fillSSID(){document.getElementById('ssid').value=document.getElementById('ssid_list').value}
async function connect(e){
  e.preventDefault();
  let s=document.getElementById('status');
  let ssid=document.getElementById('ssid').value.trim();
  let pass=document.getElementById('pass').value;
  if(!ssid){s.textContent='Please enter a WiFi name';return}
  if(pass.length>0 && pass.length<8){s.textContent='Password too short (min 8 chars)';return}
  s.textContent='Connecting...';
  try{let r=await fetch('/connect?ssid='+encodeURIComponent(ssid)+'&pass='+encodeURIComponent(pass));
      let j=await r.json();
      if(j.ok) s.textContent='Saved! Rebooting...';
      else s.textContent='Error: '+j.msg}
  catch(err){s.textContent='Connection failed'}}
scan();
</script>
</body>
</html>
)raw";

WifiMgr& WifiMgr::instance() {
    static WifiMgr mgr;
    return mgr;
}

bool WifiMgr::begin() {
    // NVS 中读取凭据（先检查 key，避免首次启动报 NOT_FOUND 错误日志）
    Preferences prefs;
    prefs.begin(NVS_WIFI_NS, true);
    String savedSSID, savedPass;
    if (prefs.isKey(NVS_KEY_SSID)) {
        savedSSID = prefs.getString(NVS_KEY_SSID, "");
        savedPass = prefs.getString(NVS_KEY_PASS, "");
    }
    prefs.end();

    if (savedSSID.length() > 0) {
        Serial.printf("[WiFi] 尝试连接: %s\n", savedSSID.c_str());
        WiFi.mode(WIFI_STA);
        WiFi.begin(savedSSID.c_str(), savedPass.c_str());

        _state = WifiState::CONNECTING;
        _connStart = millis();

        // 同步等待连接（最多 WIFI_CONN_TIMEOUT ms）
        wl_status_t lastStatus = WL_IDLE_STATUS;
        while (WiFi.status() != WL_CONNECTED && (millis() - _connStart) < WIFI_CONN_TIMEOUT) {
            if (WiFi.status() != lastStatus) {
                lastStatus = WiFi.status();
                Serial.printf("[WiFi] 状态: %d\n", lastStatus);
            }
            delay(100);
        }

        if (WiFi.status() == WL_CONNECTED) {
            snprintf(_ip, sizeof(_ip), "%s", WiFi.localIP().toString().c_str());
            Serial.printf("[WiFi] 已连接: %s\n", _ip);
            _state = WifiState::CONNECTED;
            return true;
        }

        // 连接失败 → 诊断原因
        //   0=IDLE  1=NO_SSID  4=CONNECT_FAIL(通常密码错)
        Serial.printf("[WiFi] 连接失败, status=%d", lastStatus);
        if (lastStatus == WL_NO_SSID_AVAIL) {
            Serial.println(" (WiFi 名称找不到)");
        } else if (lastStatus == WL_CONNECT_FAILED) {
            Serial.println(" (连接失败: 检查密码/信号/路由器)");
        } else {
            Serial.println();
        }
        Serial.println("[WiFi] 清除凭据，进入配网");
        Preferences clearPrefs;
        clearPrefs.begin(NVS_WIFI_NS, false);
        clearPrefs.clear();
        clearPrefs.end();
    }

    startAP();
    return false;
}

void WifiMgr::tick() {
    if (_apMode && _dns) {
        _dns->processNextRequest();
    }
}

void WifiMgr::startAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
    snprintf(_ip, sizeof(_ip), "%s", WiFi.softAPIP().toString().c_str());

    // DNS 劫持：所有域名 → ESP32 IP
    _dns = new DNSServer();
    _dns->start(53, "*", WiFi.softAPIP());
    _apMode = true;
    _state = WifiState::AP_MODE;

    Serial.printf("[WiFi] 配网热点: %s / IP: %s\n", WIFI_AP_SSID, _ip);
}

void WifiMgr::forget() {
    Preferences prefs;
    prefs.begin(NVS_WIFI_NS, false);
    prefs.clear();
    prefs.end();
    Serial.println("[WiFi] 凭据已清除，重启进入配网");
    ESP.restart();
}

// --- 供 WebServer 回调使用 ------------------------------------
bool WifiMgr::saveAndReboot(const char* ssid, const char* pass) {
    // 校验
    if (!ssid || strlen(ssid) == 0) return false;
    if (pass && strlen(pass) > 0 && strlen(pass) < 8) return false;  // WPA2 最低 8 位

    Preferences prefs;
    prefs.begin(NVS_WIFI_NS, false);
    prefs.putString(NVS_KEY_SSID, ssid);
    prefs.putString(NVS_KEY_PASS, pass ? pass : "");
    prefs.end();

    Serial.printf("[WiFi] 凭据已保存: %s，3 秒后重启...\n", ssid);
    delay(500);
    ESP.restart();
    return true;
}
