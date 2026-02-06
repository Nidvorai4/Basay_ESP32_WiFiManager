#include "Basay_WiFi_mngr.h"
#include <Preferences.h>


Preferences wifiPrefs;

void BasayWiFiManager::begin(const char* apName, const char* hostName) {
    _apName= String(apName);
    BASAY_LOGLN("\n[WM] --- Start ---");
    
    uint64_t mac = ESP.getEfuseMac();
    String macId = String((uint32_t)(mac >> 32), HEX) + String((uint32_t)mac, HEX);
    _finalHostname = String(hostName) + "_" + macId.substring(macId.length() - 4);
    _finalHostname.toLowerCase();
    // Заменяем всё, что не буквы и не цифры, на дефис
    for (int i = 0; i < _finalHostname.length(); i++) {
        char c = _finalHostname[i];
        if (!( (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') )) {
            _finalHostname[i] = '-';
        }
    }

    wifiPrefs.begin("basay_wifi", true);
    String wifi_name = wifiPrefs.getString("Basay_WiFi_Name", "");
    String wifi_pass = wifiPrefs.getString("Basay_WiFi_PSWD", "");
    _basayWiFi_ShowCaptivePortal = wifiPrefs.getBool("BSY_WiFi_ShCaPo", true);
    wifiPrefs.end();

    /*пытаемся подключиться к сохранённой сети*/
    if (wifi_name != "") {
        BASAY_LOGF("[WM] Try to connect to STA: %s\n", wifi_name.c_str());
        WiFi.mode(WIFI_STA);
        WiFi.setHostname(_finalHostname.c_str()); 
        WiFi.begin(wifi_name.c_str(), wifi_pass.c_str());
        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) 
            { delay(500); BASAY_LOG(".");}
        BASAY_LOGLN("");
    }

    if (!_endpoints_reserved_allready){
        // ОБЩИЕ ЭНДПОЙНТЫ
        server.on("/BasayWiFi_reset", HTTP_GET, [this](AsyncWebServerRequest *r){
                r->send(200, "text/plain", "WiFi Reset. Rebooting...");
                this->resetSettings();
        });
        server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *r){ r->send(204); });
        server.on("/BasayWiFi_scan", HTTP_GET, [](AsyncWebServerRequest *r){
        int n = WiFi.scanComplete();
        if (n == -2) {
            // ХИТРОСТЬ: Перед сканом принудительно останавливаем фоновые попытки подключения
            // Это освобождает радиомодуль для быстрого сканирования
            if (WiFi.status() != WL_CONNECTED) {
                WiFi.disconnect(); 
            } 
            WiFi.scanNetworks(true); r->send(202); 
        }
        else if (n == -1) { r->send(202); }
        else {
            String json = "[";
            for (int i = 0; i < n; ++i) {
                if (i > 0) json += ",";
                String ssid = WiFi.SSID(i);
                ssid.replace("\"", "'"); //экранирование несъедобных для джейсона кавычек
                json += "{\"ssid\":\"" + ssid + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
            }
            json += "]";
            r->send(200, "application/json", json); WiFi.scanDelete();
            
        }
        });

        server.on("/BasayWiFiSetup", HTTP_GET, [this](AsyncWebServerRequest *r){ 
            String page = String(WiFiSetup_html);
            // Заменяем плейсхолдер на реальное имя хоста
            page.replace("{{HOSTNAME}}", this->_finalHostname);
            r->send(200, "text/html", page); 
        });

        server.on("/BasayWiFi_save", HTTP_POST, [this](AsyncWebServerRequest *r){
            String targetSSID = r->hasArg("s_select") && r->arg("s_select") != "" ? r->arg("s_select") : r->arg("s_manual");
            targetSSID.trim();
            String targetPass = r->arg("p");
            targetPass.trim();
            if (targetSSID != "") {
                Preferences p_write; p_write.begin("basay_wifi", false);
                p_write.putString("Basay_WiFi_Name", targetSSID); p_write.putString("Basay_WiFi_PSWD", targetPass);
                p_write.end();
                r->send(200, "text/html", "Saved. Rebooting..."); 
                _shouldReboot = true; //хэндл сам ребутнёт
            } else r->send(400);
        });

        server.on("/Basay_NoCP_Save", HTTP_GET, [this](AsyncWebServerRequest *r){
            bool val = r->hasArg("v") && r->arg("v") == "1";
            Preferences p_save; p_save.begin("basay_wifi", false);
            p_save.putBool("BSY_WiFi_ShCaPo", val);
            p_save.end();
            this->_basayWiFi_ShowCaptivePortal = val;
            r->send(200);
        });


        server.on("/BasayWiFi_GoApp", HTTP_GET, [this](AsyncWebServerRequest *r){
            this->_isWeStillNeedCaptiveMode = false;
            _dnsServer.stop();
            r->redirect("/"); 
        });

        server.on("/BasayWiFi_AP_WelcomeScreen", HTTP_GET, [this](AsyncWebServerRequest *r){
            BASAY_LOGLN("Kajem WelcomeScreen");
            String page = String(BasayWiFi_AP_WelcomeScreen_html);
            page.replace("BASAY_LOCAL_IP_ADDR", BASAY_WIFI_IP_STR);
            r->send(200, "text/html", page);
        });


        // Костыль для Android
        server.on("/generate_204", HTTP_GET, [this](AsyncWebServerRequest *r){
            if (this->_isWeStillNeedCaptiveMode) r->redirect("http://" + String(BASAY_WIFI_IP_STR) + "/BasayWiFi_AP_WelcomeScreen");
            else  r->send(204); 
        });

        // Костыль для iOS
        server.on("/hotspot-detect.html", HTTP_GET, [this](AsyncWebServerRequest *r){
            if (this->_isWeStillNeedCaptiveMode) r->redirect("http://" + String(BASAY_WIFI_IP_STR) + "/BasayWiFi_AP_WelcomeScreen");
            else r->send(200, "text/html", "<html><body>Success</body></html>"); 
        });    

        server.onNotFound([this](AsyncWebServerRequest *r){ 
            if ( WiFi.getMode() == WIFI_STA || !_basayWiFi_ShowCaptivePortal) {
                r->send(404);
            } else {
                r->redirect("http://" + String(BASAY_WIFI_IP_STR) + "/BasayWiFi_AP_WelcomeScreen"); 
            }
        }); 
    
    _endpoints_reserved_allready = true;
    }

    
    if (WiFi.status() == WL_CONNECTED) {
        BASAY_LOGLN("[WM] STA Connected");
        if (MDNS.begin(_finalHostname.c_str())) MDNS.addService("http", "tcp", 80);
        
    } else {
        BASAY_LOGLN("[WM] STA Fail. AP Mode On...");
        _setupAP(wifi_name != "");
        
    }
    server.begin();
    
}

void BasayWiFiManager::handle() { 
    if ((WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) && _isWeStillNeedCaptiveMode) {
        _dnsServer.processNextRequest(); 
    }
    _updateWiFiState();
    
    if (_shouldReboot) {
        delay(999); // Даём время на отправку последнего HTTP пакета
        ESP.restart();
    }
}

/**
 * @brief Инициализация точки доступа (AP)
 * @param forceWelcomeScreen Если true, принудительно показывает Welcome Screen 
 *                           даже если Captive Portal отключён в настройках.
 *                           Полезно, когда сохраненная сеть стала недоступна.
 */
void BasayWiFiManager::_setupAP(bool wiFiFailed) {
    BASAY_LOGLN("[WM] Setting up AP Mode...");
    // Если это принудительный режим и юзер задал коллбэк — вызываем его
    if (wiFiFailed && _RouterWiFi_disappearCallback != nullptr) {
        _RouterWiFi_disappearCallback(); 
    }
    // если имя вафли отсутствует (из вызова бегина) - не пытаемся в неё долбиться
    WiFi.mode(wiFiFailed ? WIFI_AP_STA : WIFI_AP);

    IPAddress apIP;
    apIP.fromString(BASAY_WIFI_IP_STR);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(_apName.c_str()); 
    
    //если вафля падала - обязательно кажем вэлком. Если нет - как пользователь скажет
    _isWeStillNeedCaptiveMode = wiFiFailed?true:_basayWiFi_ShowCaptivePortal;

    if (_isWeStillNeedCaptiveMode){
        _dnsServer.start(53, "*", apIP);
        BASAY_LOGLN("[WM] Captive Portal Active");
    }else{
        _dnsServer.stop();
        BASAY_LOGLN("[WM] Captive Portal Disabled by user");
    }


}

void BasayWiFiManager::_updateWiFiState() {
    // Проверяем состояние раз в 5 секунд, чтобы не нагружать процессор
    if (millis() - _wifiCheckTimer < 5000) return;
    _wifiCheckTimer = millis();

    uint8_t mode = WiFi.getMode();
    uint8_t status = WiFi.status();

    // СИТУАЦИЯ А: Мы в режиме AP_STA, и интернет ПОЯВИЛСЯ
    if (mode == WIFI_AP_STA && status == WL_CONNECTED) {
        if (_staConnectedTime == 0) {
            _staConnectedTime = millis();
            
        }
        // Если стабильно подключены более 30 секунд — гасим точку доступа
        if (millis() - _staConnectedTime > 30000) {
            BASAY_LOGLN("[WM] STA Stable. Turning off AP...");
            _dnsServer.stop();
            WiFi.mode(WIFI_STA);
            //ОЖИВЛЯЕМ MDNS ---
            if (!MDNS.begin(_finalHostname.c_str())) {
                BASAY_LOGLN("[WM] Error setting up MDNS!");
            } else {
                MDNS.addService("http", "tcp", 80);
                BASAY_LOGLN("[WM] mDNS restarted and announced");
            } 
            if (_RouterWiFi_connectedCallback != nullptr) _RouterWiFi_connectedCallback();
            _staConnectedTime = 0;
        }
        return;
    } 
    // СИТУАЦИЯ Б: Мы были в STA, но интернет ПРОПАЛ (роутер выключили)
    if (mode == WIFI_STA && status != WL_CONNECTED) {
        BASAY_LOGLN("[WM] Connection lost. Enabling Recovery AP...");
        _setupAP(true);
        _staConnectedTime = 0;
        return;
    }
    // СИТУАЦИЯ В: Если просто подключены, сбрасываем счетчик стабильности
    if (status != WL_CONNECTED) {
        _staConnectedTime = 0;
    }
}
void BasayWiFiManager::resetSettings() { 
    wifiPrefs.begin("basay_wifi", false); wifiPrefs.clear(); wifiPrefs.end(); delay(500); ESP.restart(); 
}

bool BasayWiFiManager::isRouterConnected()
{
    return WiFi.getMode() == WIFI_STA;
}

BasayWiFiManager BasayWiFi;
