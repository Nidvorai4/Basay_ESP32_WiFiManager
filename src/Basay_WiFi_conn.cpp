#include "Basay_WiFi_conn.h"
#include <Preferences.h>


AsyncWebServer server(80);
DNSServer dnsServer;
Preferences wifiPrefs;

bool BasayWiFiManager::begin(const char* apName, const char* hostName, const char* rebootMsg) {
    _rebootMsg = rebootMsg;
    BASAY_LOGLN("\n[WM] --- Start ---");
    
    uint64_t mac = ESP.getEfuseMac();
    String macId = String((uint32_t)(mac >> 32), HEX) + String((uint32_t)mac, HEX);
    _finalHostname = String(hostName) + "_" + macId.substring(macId.length() - 4);
    _finalHostname.toLowerCase();

    wifiPrefs.begin("basay_wifi", true);
    String s = wifiPrefs.getString("Basay_WiFi_Name", "");
    String p = wifiPrefs.getString("Basay_WiFi_PSWD", "");
    wifiPrefs.end();

    if (s != "") {
        BASAY_LOGF("[WM] Connecting to STA: %s\n", s.c_str());
        WiFi.mode(WIFI_STA);
        WiFi.setHostname(_finalHostname.c_str()); 
        WiFi.begin(s.c_str(), p.c_str());
        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) { 
            delay(500); BASAY_LOG("."); 
        }
        BASAY_LOGLN("");
    }

    server.on("/BasayWiFi_reset", HTTP_GET, [this](AsyncWebServerRequest *r){
        r->send(200, "text/plain", "Resetting...");
        this->resetSettings();
    });

    if (WiFi.status() == WL_CONNECTED) {
        BASAY_LOGLN("[WM] STA Connected");
        if (MDNS.begin(_finalHostname.c_str())) MDNS.addService("http", "tcp", 80);
        return true; 
    } else {
        BASAY_LOGLN("[WM] STA Fail. AP Mode On...");
        WiFi.mode(WIFI_AP);
        IPAddress apIP;
        apIP.fromString(BASAY_WIFI_IP_STR);
        WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
        WiFi.softAP(apName);
        dnsServer.start(53, "*", apIP);

        server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *r){ r->send(204); });

        server.on("/BasayWiFi_CloseCaptivePortal", HTTP_GET, [this](AsyncWebServerRequest *r){
            this->isCaptiveModeDisabled = true; 
            BASAY_LOGLN("[WM] CAPTIVE PORTAL DISABLED: Unlocking access");
            r->send(204); 
        });

        server.on("/BasayWiFi_AP_WelcomeScreen", HTTP_GET, [this](AsyncWebServerRequest *r){
            String page = String(BasayWiFi_AP_WelcomeScreen_html);
            page.replace("{HOSTNAME}", _finalHostname);
            page.replace("BASAY_LOCAL_IP_ADDR", BASAY_WIFI_IP_STR);
            r->send(200, "text/html", page);
        });

        server.on("/BasayWiFiSetup", HTTP_GET, [](AsyncWebServerRequest *r){ 
            r->send(200, "text/html", WiFiSetup_html); 
        });

        server.on("/BasayWiFi_scan", HTTP_GET, [](AsyncWebServerRequest *r){
            int n = WiFi.scanComplete();
            if (n == -2) { WiFi.scanNetworks(true); r->send(202); }
            else {
                String json = "[";
                for (int i = 0; i < n; ++i) {
                    if (i > 0) json += ",";
                    json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
                }
                json += "]";
                WiFi.scanDelete(); 
                r->send(200, "application/json", json);
            }
        });

        server.on("/BasayWiFi_save", HTTP_POST, [this](AsyncWebServerRequest *r){
            String targetSSID = r->hasArg("s_select") && r->arg("s_select") != "" ? r->arg("s_select") : r->arg("s_manual");
            if (targetSSID != "") {
                Preferences p_write; p_write.begin("basay_wifi", false);
                p_write.putString("Basay_WiFi_Name", targetSSID); 
                p_write.putString("Basay_WiFi_PSWD", r->arg("p"));
                p_write.end();
                r->send(200, "text/html", "Saved. Rebooting..."); 
                delay(1000); ESP.restart();
            } else r->send(400);
        });

        server.on("/generate_204", HTTP_GET, [this](AsyncWebServerRequest *r){
            if (this->isCaptiveModeDisabled) r->send(204); 
            else r->redirect("/BasayWiFi_AP_WelcomeScreen");
        });

        server.on("/hotspot-detect.html", HTTP_GET, [this](AsyncWebServerRequest *r){
            if (this->isCaptiveModeDisabled) r->send(200, "text/html", "Success"); 
            else r->redirect("/BasayWiFi_AP_WelcomeScreen");
        });

        server.onNotFound([this](AsyncWebServerRequest *r){ 
            if (this->isCaptiveModeDisabled) {
                if (r->url().indexOf("generate_204") >= 0) r->send(204);
                else r->send(404);
            } else r->redirect("/BasayWiFi_AP_WelcomeScreen"); 
        });

        return false;
    }
}

void BasayWiFiManager::handle() { 
    if (WiFi.getMode() == WIFI_AP && !isCaptiveModeDisabled) {
        dnsServer.processNextRequest(); 
    }
}

void BasayWiFiManager::resetSettings() { 
    wifiPrefs.begin("basay_wifi", false); 
    wifiPrefs.clear(); 
    wifiPrefs.end(); 
    delay(500); 
    ESP.restart(); 
}

BasayWiFiManager BasayWiFi;
