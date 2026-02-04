#include "Basay_WiFi_man.h"
#include <Preferences.h>

AsyncWebServer server(80);
DNSServer dnsServer;
Preferences wifiPrefs;

bool BasayWiFiManager::begin(const char* apName, const char* hostName) {
    BASAY_LOGLN("\n[WM] --- Start ---");
    
    uint64_t mac = ESP.getEfuseMac();
    String macId = String((uint32_t)(mac >> 32), HEX) + String((uint32_t)mac, HEX);
    _finalHostname = String(hostName) + "_" + macId.substring(macId.length() - 4);
    _finalHostname.toLowerCase();

    wifiPrefs.begin("basay_wifi", true);
    String s = wifiPrefs.getString("Basay_WiFi_Name", "");
    String p = wifiPrefs.getString("Basay_WiFi_PSWD", "");
    BasayWiFi_ShowCaptivePortal = wifiPrefs.getBool("BSY_WiFi_ShCaPo", true);
    wifiPrefs.end();

    if (s != "") {
        BASAY_LOGF("[WM] Connecting to STA: %s\n", s.c_str());
        WiFi.mode(WIFI_STA);
        WiFi.setHostname(_finalHostname.c_str()); 
        WiFi.begin(s.c_str(), p.c_str());
        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) { delay(500); BASAY_LOG(".");}
        BASAY_LOGLN("");
    }

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
        
        // Запускаем DNS только если не стоит галка "Не показывать"
        if (BasayWiFi_ShowCaptivePortal) {
            BASAY_LOGLN("BasayWiFi_ShowCaptivePortal == TRUE. Startuem");
            dnsServer.start(53, "*", apIP);
        } else {
            BASAY_LOGLN("Captive portal disabled");
            isCaptiveModeDisabled = true;
        }

        // Эндпоинты либы
        server.on("/BasayWiFi_reset", HTTP_GET, [this](AsyncWebServerRequest *r){
            r->send(200, "text/plain", "WiFi Reset. Rebooting...");
            this->resetSettings();
        });

        server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *r){ r->send(204); });

        server.on("/Basay_NoCP_Save", HTTP_GET, [this](AsyncWebServerRequest *r){
            bool val = r->hasArg("v") && r->arg("v") == "1";
            Preferences p_save; p_save.begin("basay_wifi", false);
            p_save.putBool("BSY_WiFi_ShCaPo", val);
            p_save.end();
            this->BasayWiFi_ShowCaptivePortal = val;
            r->send(200);
        });

        server.on("/BasayWiFi_GoApp", HTTP_GET, [this](AsyncWebServerRequest *r){
            this->isCaptiveModeDisabled = true;
            dnsServer.stop();
            r->redirect("/"); 
        });

        server.on("/BasayWiFi_AP_WelcomeScreen", HTTP_GET, [this](AsyncWebServerRequest *r){
            BASAY_LOGLN("Kajem WelcomeScreen");
            String page = String(BasayWiFi_AP_WelcomeScreen_html);
            page.replace("BASAY_LOCAL_IP_ADDR", BASAY_WIFI_IP_STR);
            r->send(200, "text/html", page);
        });

        server.on("/BasayWiFiSetup", HTTP_GET, [this](AsyncWebServerRequest *r){ 
            String page = String(WiFiSetup_html);
            // Заменяем плейсхолдер на реальное имя хоста
            page.replace("{{HOSTNAME}}", this->_finalHostname);
            r->send(200, "text/html", page); 
        });

        server.on("/BasayWiFi_save", HTTP_POST, [this](AsyncWebServerRequest *r){
            String targetSSID = r->hasArg("s_select") && r->arg("s_select") != "" ? r->arg("s_select") : r->arg("s_manual");
            if (targetSSID != "") {
                Preferences p_write; p_write.begin("basay_wifi", false);
                p_write.putString("Basay_WiFi_Name", targetSSID); p_write.putString("Basay_WiFi_PSWD", r->arg("p"));
                p_write.end();
                r->send(200, "text/html", "Saved. Rebooting..."); 
                delay(1000); ESP.restart();
            } else r->send(400);
        });

        // Костыль для Android
        server.on("/generate_204", HTTP_GET, [this](AsyncWebServerRequest *r){
            if (this->isCaptiveModeDisabled) r->send(204); 
            else r->redirect("http://" + String(BASAY_WIFI_IP_STR) + "/BasayWiFi_AP_WelcomeScreen");
        });

        // Костыль для iOS
        server.on("/hotspot-detect.html", HTTP_GET, [this](AsyncWebServerRequest *r){
            if (this->isCaptiveModeDisabled) r->send(200, "text/html", "<html><body>Success</body></html>"); 
            else r->redirect("http://" + String(BASAY_WIFI_IP_STR) + "/BasayWiFi_AP_WelcomeScreen");
        });





         server.onNotFound([this](AsyncWebServerRequest *r){ 
            if (this->isCaptiveModeDisabled) {
                r->send(404);
            } else {
                r->redirect("http://" + String(BASAY_WIFI_IP_STR) + "/BasayWiFi_AP_WelcomeScreen"); 
            }
        }); 
/*
        server.onNotFound([this](AsyncWebServerRequest *r){ 
            if (this->isCaptiveModeDisabled) {
                if (r->url().indexOf("generate_204") >= 0) r->send(204);
                else r->send(404);
            } else r->redirect("/BasayWiFi_AP_WelcomeScreen"); 
        });*/

        return false;
    }
}

void BasayWiFiManager::handle() { 
    if (WiFi.getMode() == WIFI_AP && !isCaptiveModeDisabled) dnsServer.processNextRequest(); 
}

void BasayWiFiManager::resetSettings() { 
    wifiPrefs.begin("basay_wifi", false); wifiPrefs.clear(); wifiPrefs.end(); delay(500); ESP.restart(); 
}

BasayWiFiManager BasayWiFi;
