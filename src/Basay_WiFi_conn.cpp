#include "Basay_WiFi_conn.h"
#include <Preferences.h>

AsyncWebServer server(80);
DNSServer dnsServer;
Preferences wifiPrefs;

bool BasayWiFiManager::begin(const char* apName, const char* hostName, const char* rebootMsg, const char* customHtml) {
    _rebootMsg = rebootMsg;
    _userHtml = customHtml;
    
    uint64_t mac = ESP.getEfuseMac();
    String macId = String((uint32_t)(mac >> 32), HEX);
    macId += String((uint32_t)mac, HEX);
    _finalHostname = String(hostName) + "_" + macId.substring(macId.length() - 4);
    _finalHostname.toLowerCase();

    wifiPrefs.begin("basay_wifi", true);
    String s = wifiPrefs.getString("Basay_WiFi_Name", "");
    String p = wifiPrefs.getString("Basay_WiFi_PSWD", "");
    wifiPrefs.end();

    if (s != "") {
        WiFi.mode(WIFI_STA);
        WiFi.setHostname(_finalHostname.c_str()); 
        WiFi.begin(s.c_str(), p.c_str());
        
        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) { delay(500); BASAY_LOG("."); }
    }

    if (WiFi.status() == WL_CONNECTED) {
        if (MDNS.begin(_finalHostname.c_str())) MDNS.addService("http", "tcp", 80);
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *r){ r->send(200, "text/html", WiFiConnectedIndex_html); });
        server.on("/reset_now", HTTP_GET, [this](AsyncWebServerRequest *r){
            r->send(200, "text/plain", "Resetting...");
            this->resetSettings();
        });
        return true;
    } else {
        WiFi.mode(WIFI_AP);
        IPAddress apIP(192, 168, 4, 1);
        WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
        WiFi.softAP(apName);
        dnsServer.start(53, "*", apIP);

        // Главная страница: либо пользовательская, либо стандартная
        server.on("/", HTTP_GET, [this](AsyncWebServerRequest *r){ 
            if(_userHtml) r->send(200, "text/html", _userHtml);
            else r->send(200, "text/html", WorkWithoutWiFi_html); 
        });

        // Новый уникальный путь для настройки
        server.on("/BasayWiFiSetup", HTTP_GET, [](AsyncWebServerRequest *r){ r->send(200, "text/html", WiFiSetup_html); });

        // Исправленные ловушки: теперь кидают на главную (на страницу пользователя)
        server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *r){ r->redirect("/"); });
        server.on("/gen_204", HTTP_GET, [](AsyncWebServerRequest *r){ r->redirect("/"); });
        server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *r){ r->redirect("/"); });

        server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *r){
            int n = WiFi.scanComplete();
            if (n == -2) { WiFi.scanNetworks(true); r->send(202); }
            else if (n == -1) { r->send(202); }
            else {
                String json = "[";
                for (int i = 0; i < n; ++i) {
                    if (i > 0) json += ",";
                    json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
                }
                json += "]";
                WiFi.scanDelete(); r->send(200, "application/json", json);
            }
        });

        server.on("/save", HTTP_POST, [this](AsyncWebServerRequest *r){
            String targetSSID = r->hasArg("s_select") && r->arg("s_select") != "" ? r->arg("s_select") : r->arg("s_manual");
            if (targetSSID != "") {
                Preferences p_write; p_write.begin("basay_wifi", false);
                p_write.putString("Basay_WiFi_Name", targetSSID); p_write.putString("Basay_WiFi_PSWD", r->arg("p"));
                p_write.end();
                
                String fullAddr = "http://" + _finalHostname + ".local";
                String h = "<!DOCTYPE HTML><html style='background:#121212;color:#e0e0e0;font-family:sans-serif;text-align:center;padding:20px;'>";
                h += "<body style='display:flex;justify-content:center;align-items:center;height:90vh;'><div style='background:#1e1e1e;padding:30px;border-radius:20px;border:1px solid #444;max-width:400px;'>";
                h += "<h2>Запомни адрес!</h2><p>" + String(_rebootMsg) + "</p>";
                h += "<div style='background:#000;padding:10px;border-radius:10px;margin:20px 0;border:1px dashed #3f8cff;position:relative;'>";
                h += "<span id='addr' style='color:#3f8cff;font-size:18px;word-break:break-all;'>" + fullAddr + "</span>";
                h += "<br><small id='copyBtn' onclick='copyToClipboard()' style='color:#ff9f43;cursor:pointer;text-decoration:underline;font-size:12px;'>[копировать]</small></div>";
                h += "<button onclick=\"this.disabled=true;this.innerHTML='Жди...';fetch('/do_reboot')\" style='background:#28a745;color:white;border:none;padding:15px 30px;border-radius:10px;font-size:20px;cursor:pointer;width:100%;'>Я ПОНЯЛ, ЖМИ!</button></div>";
                h += "<script>function copyToClipboard(){var t=document.createElement('input');t.value='"+fullAddr+"';document.body.appendChild(t);t.select();document.execCommand('copy');document.body.removeChild(t);";
                h += "var b=document.getElementById('copyBtn');b.innerHTML='СКОПИРОВАНО!';b.style.color='#28a745';setTimeout(()=>{b.innerHTML='[копировать]';b.style.color='#ff9f43';},2000);}</script></body></html>";
                r->send(200, "text/html", h);
            } else r->send(400);
        });

        server.on("/do_reboot", HTTP_GET, [](AsyncWebServerRequest *r){ r->send(200, "text/plain", "OK"); delay(500); ESP.restart(); });
        server.onNotFound([](AsyncWebServerRequest *r){ r->redirect("/"); });
        return false;
    }
}

void BasayWiFiManager::handle() { if (WiFi.getMode() == WIFI_AP) dnsServer.processNextRequest(); }
void BasayWiFiManager::resetSettings() { wifiPrefs.begin("basay_wifi", false); wifiPrefs.clear(); wifiPrefs.end(); delay(500); ESP.restart(); }

BasayWiFiManager BasayWiFi;
