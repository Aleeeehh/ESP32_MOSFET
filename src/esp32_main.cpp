#include <Arduino.h>

namespace esp32 {
const int ledPin = 35; // LED interno esp32
// const int ledPin = 4; // Lucetta integrata ESP32-CAM per flash

void blinkLed() {
  digitalWrite(ledPin, LOW);  // Accendi il LED (logica invertita)
  delay(200);                 // Aspetta 2000 ms
  digitalWrite(ledPin, HIGH); // Spegni il LED (logica invertita)
  delay(200);                 // Aspetta 2000 ms
}

void setupLed() { pinMode(ledPin, OUTPUT); }
} // namespace esp32