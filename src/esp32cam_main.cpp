#include <Arduino.h>

namespace esp32cam {
const int ledPin = 2; // LED flash ESP32-CAM

void blinkLed() {
  digitalWrite(ledPin, LOW);
  delay(1000);
  digitalWrite(ledPin, HIGH);
  delay(1000);
}

void setupLed() { pinMode(ledPin, OUTPUT); }

} // namespace esp32cam