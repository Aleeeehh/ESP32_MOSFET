#include <Arduino.h>

namespace esp32
{
  const int ledPin = 35; // LED alternativo su alcune schede ESP32
  // const int ledPin = 4; // Lucetta integrata ESP32-CAM per flash

  void blinkLed()
  {
    digitalWrite(ledPin, HIGH); // Accendi il LED
    delay(2000);                // Aspetta 2000 ms
    digitalWrite(ledPin, LOW);  // Spegni il LED
    delay(2000);                // Aspetta 2000 ms
  }

  void setupLed()
  {
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW); // Inizia spento
  }
} // namespace esp32