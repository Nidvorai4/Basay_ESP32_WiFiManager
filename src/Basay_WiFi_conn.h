#ifndef BASAY_WIFI_CONN_H
#define BASAY_WIFI_CONN_H

#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "WiFi_connection.html"

// Раскомментируй эту строку, чтобы видеть отладку в Serial порту
//#define BASAY_DEBUG 

#ifdef BASAY_WIFI_CONN_DEBUG
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

/**
 * @brief Менеджер Wi-Fi подключений с поддержкой Captive Portal и mDNS.
 */
class BasayWiFiManager {
public:
    /**
     * @brief Инициализация сетевых настроек. 
     * @param apName Имя точки доступа ESP.
     * @param hostName Имя устройства в локальной сети (http://hostName_xxxx.local).
     * @param rebootMsg Сообщение пользователю перед перезагрузкой.
     */
    bool begin(const char* apName = "Basay_Node_Config", 
               const char* hostName = "basay", 
               const char* rebootMsg = "Я ухожу в домашнюю сеть! Ищи меня по адресу:");

    void handle(); 
    void resetSettings();

private:
    String _finalHostname;
    const char* _rebootMsg;
};

extern BasayWiFiManager BasayWiFi;

#endif
