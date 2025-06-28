#include "esp32cam_main.h"
#include "webserver_main.h"
#include <Arduino.h>

void setup() {
  // esp32cam::setupLed();
  Serial.begin(115200);

  webserver::setupCamera();
  webserver::setupWiFi();
  webserver::setupServer();
}

void loop() {
  // esp32cam::blinkLed();
  webserver::loop();
}
