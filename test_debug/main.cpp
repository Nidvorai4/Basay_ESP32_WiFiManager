#include <Arduino.h>
#include "Basay_WiFi_conn.h"

unsigned long imAlive;


void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println ("Стартанулося");


    // Инициализация либы. Если вернет true — мы в домашней сети.
    bool isHome = BasayWiFi.begin("ESP_C3_TEST", "basay_test");

    // Регистрируем корень. Либа его больше не трогает, он твой.
    server.on("/", HTTP_GET, [isHome](AsyncWebServerRequest *r) {
        if (isHome) {
            r->send(200, "text/plain", "STA OK");
            Serial.println ("STA");
        } else {
            r->send(200, "text/plain", "AP APP OK (69.69.69.69)");
            Serial.println ("AP");
        }
    });

    server.begin();
}

void loop() {
    BasayWiFi.handle();
    if (millis() - imAlive > 10000){
        imAlive=millis();
        Serial.println ("im alive");
    }
}
