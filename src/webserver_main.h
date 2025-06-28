#ifndef WEBSERVER_MAIN_H
#define WEBSERVER_MAIN_H

#include <WebServer.h>
#include <WiFi.h>

namespace webserver {
void setupCamera();
void setupWiFi();
void setupServer();
void handleRoot();
void handleCapture();
void handlePhoto();
void handleStream();
void handleVideoStream();
void loop();
} // namespace webserver

#endif