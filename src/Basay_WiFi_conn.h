#ifndef BASAY_WIFI_CONN_H
#define BASAY_WIFI_CONN_H

#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "WiFi_connection.html"

//#define BASAY_DEBUG 

#ifdef BASAY_WIFI_MANAGER_DEBUG
  #define BASAY_LOG(x) Serial.print(x)
  #define BASAY_LOGLN(x) Serial.println(x)
  #define BASAY_LOGF(...) Serial.printf(__VA_ARGS__)
#else
  #define BASAY_LOG(x)
  #define BASAY_LOGLN(x)
  #define BASAY_LOGF(...)
#endif

extern AsyncWebServer server;
extern DNSServer dnsServer;

class BasayWiFiManager {
public:
    /**
     * @brief Инициализация Wi-Fi менеджера.
     * @param apName Имя точки доступа.
     * @param hostName Имя для mDNS (http://name_xxxx.local).
     * @param rebootMsg Текст на странице подтверждения.
     * @param customHtml Свой HTML для страницы БЕЗ вайфая. В нем должна быть ссылка на /BasayWiFiSetup
     */
    bool begin(const char* apName = "Basay_Node_Config", 
               const char* hostName = "basay", 
               const char* rebootMsg = "Я ухожу в домашнюю сеть! Ищи меня по адресу:",
               const char* customHtml = nullptr);

    void handle(); 
    void resetSettings();

private:
    String _finalHostname;
    const char* _rebootMsg;
    const char* _userHtml;
};

extern BasayWiFiManager BasayWiFi;

#endif
