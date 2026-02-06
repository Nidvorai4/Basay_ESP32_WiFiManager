#include <Arduino.h>
#include "Basay_WiFi_mngr.h"
#include "main_test.html" // Подключаем наши тестовые страницы

unsigned long imAlive;

void setup() {
    delay(5000); 
    Serial.begin(115200);
    unsigned long start = millis();
    while (!Serial && (millis() - start < 3000)) {
        delay(10);
    }
    delay(3000); 
    Serial.println("Стартанулося");

    // Инициализация либы.
    BasayWiFi.begin("ESP_C3_TEST_HUEST", "basay_test");
    
    // Регистрируем корень
    
    BasayWiFi.server.on("/", HTTP_GET,[](AsyncWebServerRequest *r) {
        if (BasayWiFi.isRouterConnected()) {
            // Отправляем красивую страницу для режима Роутера
            r->send(200, "text/html", PAGE_STA_TEST);
            Serial.println("STA: Served Page");
        } else {
            // Отправляем красивую страницу для режима Автономки
            r->send(200, "text/html", PAGE_AP_TEST);
            Serial.println("AP: Served Page");
        }
    });

    
}

void loop() {
    BasayWiFi.handle();
    
    if (millis() - imAlive > 10000) {
        imAlive = millis();
        Serial.println("im alive");
    }
}
