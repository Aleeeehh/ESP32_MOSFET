#include "esp32_main.h"
#include <Arduino.h>

void setup()
{
  esp32::setupLed();
  Serial.begin(115200);
}

void loop()
{
  esp32::blinkLed();
}
