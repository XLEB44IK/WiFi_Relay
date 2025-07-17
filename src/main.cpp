#include <Arduino.h>

// Для ESP32-C3 DevKitM-1 используем GPIO8 как встроенный светодиод
#define LED_PIN 8 // На большинстве ESP32-C3 плат светодиод на GPIO8

void setup()
{
  // Стандартная инициализация Serial для ESP32-C3
  Serial.begin(9600);

  // Ждем подключения Serial Monitor (важно для ESP32-C3 с USB-CDC)
  while (!Serial && millis() < 5000)
  {
    delay(10);
  }

  pinMode(LED_PIN, OUTPUT); // Настроить светодиод как выход

  // Дополнительная диагностическая информация
  Serial.println();
  Serial.println("=== ESP32-C3 WiFi Relay Starting ===");
  Serial.print("CPU Frequency: ");
  Serial.print(getCpuFrequencyMhz());
  Serial.println(" MHz");
  Serial.println("Serial Monitor подключен!");
  Serial.println("Светодиод должен мигать каждые 500ms");
  Serial.println("=======================================");
}

void loop()
{
  Serial.print("ALive! Time: ");          // Отправить сообщение в последовательный порт
  unsigned long currentMillis = millis(); // Получить текущее время в миллисекундах
  Serial.print(currentMillis);            // Отправить текущее время
  Serial.println(" ms");                  // Отправить новую строку

  // Мигание светодиодом
  digitalWrite(LED_PIN, HIGH); // Включить светодиод
  delay(250);                  // Подождать 250 мс
  digitalWrite(LED_PIN, LOW);  // Выключить светодиод
  delay(250);                  // Подождать 250 мс
}
