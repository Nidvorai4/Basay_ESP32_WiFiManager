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

extern AsyncWebServer server;
extern DNSServer dnsServer;

class BasayWiFiManager {
public:
    bool begin(const char* apName = "Basay_Node_Config", 
               const char* hostName = "basay");

    bool isAP() {  return (WiFi.status() != WL_CONNECTED); }
    void handle(); 
    void resetSettings();

private:
    String _finalHostname;
    const char* _rebootMsg;
    bool isCaptiveModeDisabled = false;
    bool BasayWiFi_ShowCaptivePortal = true;
};

extern BasayWiFiManager BasayWiFi;

#endif
