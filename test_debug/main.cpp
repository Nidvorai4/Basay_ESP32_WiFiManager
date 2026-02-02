#include <Basay_WiFi_conn.h>

void setup() {
    Serial.begin(115200);

    // Запуск менеджера. Он сам решит: выйти в онлайн или поднять точку доступа.
    bool isOnline = BasayWiFi.begin("My_Cool_Project", "Basay_ESP", "иди в хуй ");

    if (isOnline) {
        // РЕЖИМ С ВАЙФАЕМ: Добавляем ваши датчики из первого сообщения
        server.on("/get_sensors", HTTP_GET, [](AsyncWebServerRequest *r){
             r->send(200, "application/json", "{\"temp\": 25.4}"); 
        });
    }

    server.begin();
}

void loop() {
    BasayWiFi.handle(); // Важно для Captive Portal

    // Сброс настроек при зажатии кнопки (напр. GPIO 9)
    /* if (digitalRead(9) == LOW) { 
        delay(3000); 
        if (digitalRead(9) == LOW) BasayWiFi.resetSettings(); 
    } */
}
