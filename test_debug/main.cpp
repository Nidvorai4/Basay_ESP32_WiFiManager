#include <Arduino.h>
#include "Basay_WiFi_man.h"
#include "main_test.html" // Подключаем наши тестовые страницы

unsigned long imAlive;

void setup() {
    delay(2000);
    Serial.begin(115200);
    delay(2000);
    Serial.println("Стартанулося");

    // Инициализация либы.
    bool isHome = BasayWiFi.begin("ESP_C3_TEST_HUEST", "basay_test");

    // Регистрируем корень
    server.on("/", HTTP_GET, [isHome](AsyncWebServerRequest *r) {
        if (isHome) {
            // Отправляем красивую страницу для режима Роутера
            r->send(200, "text/html", PAGE_STA_TEST);
            Serial.println("STA: Served Page");
        } else {
            // Отправляем красивую страницу для режима Автономки
            r->send(200, "text/html", PAGE_AP_TEST);
            Serial.println("AP: Served Page");
        }
    });

    server.begin();
}

void loop() {
    BasayWiFi.handle();
    
    if (millis() - imAlive > 10000) {
        imAlive = millis();
        Serial.println("im alive");
    }
}
