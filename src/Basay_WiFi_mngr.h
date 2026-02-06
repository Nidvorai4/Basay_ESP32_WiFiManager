#ifndef BASAY_WIFI_MAN_H
#define BASAY_WIFI_MAN_H

#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "Basay_WiFi_mngr.html"


#define BASAY_WIFI_IP_STR "10.69.69.10"

//#define BASAY_DEBUG 

#ifdef BASAY_WIFI_MANAGER_DEBUG //включается в ини
  #define BASAY_LOG(x) Serial.print(x)
  #define BASAY_LOGLN(x) Serial.println(x)
  #define BASAY_LOGF(...) Serial.printf(__VA_ARGS__)
#else
  #define BASAY_LOG(x)
  #define BASAY_LOGLN(x)
  #define BASAY_LOGF(...)
#endif




class BasayWiFiManager {
public:
    AsyncWebServer server;    
    BasayWiFiManager() : server(80) {}
    // Определяем тип функции коллбэка
    typedef void (*BasayWiFi_RouterStatus_Callback)();

    //для регистрации коллбэка отвала сети пользователем
    void onRouterWiFi_disappearCallbackRegister(BasayWiFi_RouterStatus_Callback cb) { _RouterWiFi_disappearCallback = cb; }
    //для регистрации коллбэка привала сети пользователем
    void onRouterWiFi_connectedCallbackRegister(BasayWiFi_RouterStatus_Callback cb) { _RouterWiFi_connectedCallback = cb; }

    void begin(const char* apName = "Basay_Node_Config", 
               const char* hostName = "basay");

    //bool isAP() {  return (WiFi.status() != WL_CONNECTED); }
    void handle(); 
    void resetSettings();
    bool isRouterConnected ();

private:
    BasayWiFi_RouterStatus_Callback _RouterWiFi_disappearCallback = nullptr;
    BasayWiFi_RouterStatus_Callback _RouterWiFi_connectedCallback = nullptr;
   
    void _updateWiFiState();
    void _setupAP(bool wiFiFailed );
    DNSServer _dnsServer;  
    String _finalHostname;
    const char* _rebootMsg;
    bool _isWeStillNeedCaptiveMode = true;
    bool _basayWiFi_ShowCaptivePortal = true;
    bool _endpoints_reserved_allready = false;
    bool _shouldReboot = false;
    unsigned long _wifiCheckTimer = 0;
    unsigned long _staConnectedTime = 0;
    String _apName;

};

extern BasayWiFiManager BasayWiFi;

#endif
