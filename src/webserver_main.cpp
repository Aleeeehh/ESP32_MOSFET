#include "webserver_main.h"
#include "esp_camera.h"
#include <Arduino.h>

namespace webserver {
  //test commit

// Credenziali Hotspot Telefono
const char *ssid = "Iphone di Prato"; // Nome dell'hotspot del telefono
const char *password = "Ciaoo111";    // Password dell'hotspot

WebServer server(80);

// Variabile globale per memorizzare l'ultima foto
uint8_t *lastPhotoBuffer = nullptr;
size_t lastPhotoSize = 0;
unsigned long lastPhotoTimestamp = 0; // Timestamp per evitare caching

// Configurazione fotocamera ESP32-CAM
void setupCamera() {
  Serial.println("🔧 Inizializzazione fotocamera...");
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 0;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sccb_sda = 26; // Corretto: pin_sccb_sda invece di pin_sscb_sda
  config.pin_sccb_scl = 27; // Corretto: pin_sccb_scl invece di pin_sscb_scl
  config.pin_pwdn = 32;
  config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size =
      FRAMESIZE_QVGA;      // 320x240 per streaming veloce (era VGA 640x480)
  config.jpeg_quality = 5; // Qualità molto bassa per streaming veloce (era 10)
  config.fb_count = 1;     // 1 buffer per ridurre latenza (era 2)

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("❌ Errore inizializzazione fotocamera: 0x%x\n", err);
    return;
  }
  Serial.println("✅ Fotocamera inizializzata!");
  Serial.printf("📐 Risoluzione: %dx%d\n", 320, 240);
  Serial.printf("🎯 Qualità JPEG: %d\n", config.jpeg_quality);
  Serial.printf("⚡ Ottimizzata per streaming veloce!\n");
}

void setupWiFi() {
  Serial.println("📡 Connessione WiFi...");
  Serial.printf("SSID: %s\n", ssid);
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("✅ WiFi connesso!");
    Serial.printf("🌐 IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("📶 RSSI: %d dBm\n", WiFi.RSSI());
  } else {
    Serial.println("❌ Connessione WiFi fallita!");
  }
}

void handleRoot() {
  Serial.println("🏠 Richiesta pagina principale");
  String html = "<!DOCTYPE html><html><head><title>ESP32-CAM</title>";
  html += "<meta charset='UTF-8'>";
  html += "<style>";
  html += "body{font-family:Arial,sans-serif;text-align:center;margin:20px;}";
  html += ".photo-container{margin:20px;padding:10px;border:2px solid "
          "#ccc;display:inline-block;}";
  html += ".stream-container{margin:20px;padding:10px;border:2px solid "
          "#0066cc;display:inline-block;}";
  html += "button{padding:15px "
          "30px;font-size:18px;margin:10px;border:none;cursor:pointer;}";
  html += ".photo-btn{background-color:#4CAF50;color:white;}";
  html += ".photo-btn:hover{background-color:#45a049;}";
  html += ".stream-btn{background-color:#0066cc;color:white;}";
  html += ".stream-btn:hover{background-color:#0052a3;}";
  html += "</style>";
  html += "<script>";
  html += "window.onload = function() {";
  html += "  var img = document.getElementById('photo');";
  html += "  if(img) {";
  html += "    img.src = '/photo?t=' + Date.now();"; // Forza ricaricamento
  html += "  }";
  html += "};";
  html += "</script>";
  html += "</head><body>";
  html += "<h1>📷 ESP32-CAM</h1>";
  html += "<p>";
  html += "<a href='/capture'><button class='photo-btn'>📸 Scatta "
          "Foto</button></a>";
  html += "<a href='/stream'><button class='stream-btn'>🎥 Streaming "
          "Video</button></a>";
  html += "</p>";

  if (lastPhotoBuffer != nullptr && lastPhotoSize > 0) {
    html += "<div class='photo-container'>";
    html += "<h3>Ultima Foto Scattata:</h3>";
    html += "<img id='photo' src='/photo?t=" + String(lastPhotoTimestamp) +
            "' style='max-width:100%;max-height:400px;'>";
    html += "</div>";
  } else {
    html += "<p style='color:#666;'>Clicca un bottone per iniziare!</p>";
  }

  html += "</body></html>";

  server.send(200, "text/html", html);
  Serial.println("✅ Pagina principale inviata");
}

void handleCapture() {
  Serial.println("📸 Richiesta scatto foto");

  // Libera la memoria della foto precedente
  if (lastPhotoBuffer != nullptr) {
    Serial.println("🗑️ Liberazione memoria foto precedente");
    free(lastPhotoBuffer);
    lastPhotoBuffer = nullptr;
    lastPhotoSize = 0;
  }

  Serial.println("📷 Acquisizione frame...");
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("❌ Errore acquisizione foto");
    server.send(500, "text/plain", "Errore acquisizione foto");
    return;
  }

  Serial.printf("📊 Frame acquisito: %d bytes\n", fb->len);

  // Alloca memoria per salvare la foto
  lastPhotoBuffer = (uint8_t *)malloc(fb->len);
  if (lastPhotoBuffer == nullptr) {
    Serial.println("❌ Errore allocazione memoria");
    esp_camera_fb_return(fb);
    server.send(500, "text/plain", "Errore memoria");
    return;
  }

  // Copia la foto in memoria
  memcpy(lastPhotoBuffer, fb->buf, fb->len);
  lastPhotoSize = fb->len;
  lastPhotoTimestamp = millis(); // Aggiorna timestamp

  esp_camera_fb_return(fb);

  Serial.printf("✅ Foto salvata: %d bytes\n", lastPhotoSize);

  // Invia pagina con refresh automatico
  String html = "<!DOCTYPE html><html><head><title>Foto Scattata</title>";
  html += "<meta charset='UTF-8'>";
  html += "<style>";
  html += "body{font-family:Arial,sans-serif;text-align:center;margin:20px;}";
  html += ".photo-container{margin:20px;padding:10px;border:2px solid "
          "#ccc;display:inline-block;}";
  html += "button{padding:15px "
          "30px;font-size:18px;background-color:#4CAF50;color:white;border:"
          "none;cursor:pointer;}";
  html += "button:hover{background-color:#45a049;}";
  html += "</style>";
  html += "<script>";
  html +=
      "setTimeout(function(){ window.location.href='/'; }, 100);"; // Refresh
                                                                   // dopo 100ms
  html += "</script>";
  html += "</head><body>";
  html += "<h1>✅ Foto Scattata!</h1>";
  html += "<p>Aggiornamento automatico...</p>";
  html += "</body></html>";

  server.send(200, "text/html", html);
  Serial.println("✅ Pagina foto inviata");
}

void handlePhoto() {
  Serial.println("🖼️ Richiesta visualizzazione foto");

  if (lastPhotoBuffer == nullptr || lastPhotoSize == 0) {
    Serial.println("❌ Nessuna foto disponibile");
    server.send(404, "text/plain", "Nessuna foto disponibile");
    return;
  }

  Serial.printf("📤 Invio foto: %d bytes\n", lastPhotoSize);

  // Aggiungi header per evitare caching
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "0");
  server.sendHeader("Content-Disposition", "inline; filename=capture.jpg");
  server.setContentLength(lastPhotoSize);
  server.send(200, "image/jpeg", "");

  // Invia i dati in chunk piccoli per evitare problemi
  const size_t chunkSize = 1024;
  size_t totalSent = 0;

  for (size_t i = 0; i < lastPhotoSize; i += chunkSize) {
    size_t currentChunkSize =
        (i + chunkSize < lastPhotoSize) ? chunkSize : (lastPhotoSize - i);

    Serial.printf("📦 Invio chunk %d/%d: %d bytes\n", (i / chunkSize) + 1,
                  (lastPhotoSize + chunkSize - 1) / chunkSize,
                  currentChunkSize);

    server.sendContent((const char *)(lastPhotoBuffer + i), currentChunkSize);
    totalSent += currentChunkSize;

    // Piccola pausa tra i chunk
    delay(1);

    // Controlla se la connessione è ancora attiva
    if (!server.client().connected()) {
      Serial.println("❌ Client disconnesso durante invio");
      return;
    }
  }

  Serial.printf("✅ Foto inviata completamente: %d bytes\n", totalSent);
}

void handleStream() {
  Serial.println("🎥 Richiesta pagina streaming");
  String html = "<!DOCTYPE html><html><head><title>Streaming Video</title>";
  html += "<meta charset='UTF-8'>";
  html += "<style>";
  html += "body{font-family:Arial,sans-serif;text-align:center;margin:20px;}";
  html += ".stream-container{margin:20px;padding:10px;border:2px solid "
          "#0066cc;display:inline-block;}";
  html += "button{padding:15px "
          "30px;font-size:18px;background-color:#4CAF50;color:white;border:"
          "none;cursor:pointer;}";
  html += "button:hover{background-color:#45a049;}";
  html += "</style>";
  html += "</head><body>";
  html += "<h1>🎥 Streaming Video ESP32-CAM</h1>";
  html += "<div class='stream-container'>";
  html += "<img src='/video' style='max-width:100%;max-height:500px;'>";
  html += "</div>";
  html += "<p><a href='/'><button>← Torna alla home</button></a></p>";
  html += "</body></html>";

  server.send(200, "text/html", html);
  Serial.println("✅ Pagina streaming inviata");
}

void handleVideoStream() {
  Serial.println("🎬 Inizio streaming MJPEG");

  WiFiClient client = server.client();

  // Invia header HTTP per MJPEG streaming
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
  client.println("Connection: close");
  client.println();

  Serial.println("📡 Header MJPEG inviati, inizio loop streaming");

  unsigned long frameCount = 0;
  unsigned long startTime = millis();

  while (client.connected()) {
    frameCount++;
    unsigned long frameStart = millis();

    Serial.printf("📷 Frame %lu: acquisizione...\n", frameCount);

    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.printf("❌ Frame %lu: errore acquisizione\n", frameCount);
      continue;
    }

    unsigned long acquisitionTime = millis() - frameStart;
    Serial.printf("📊 Frame %lu: %d bytes (acquisizione: %lu ms)\n", frameCount,
                  fb->len, acquisitionTime);

    // Invia boundary
    client.println("--frame");

    // Invia header del frame
    client.println("Content-Type: image/jpeg");
    client.printf("Content-Length: %u\r\n\r\n", fb->len);

    Serial.printf("📤 Frame %lu: invio dati...\n", frameCount);

    // Invia i dati del frame
    client.write(fb->buf, fb->len);
    client.println();

    esp_camera_fb_return(fb);

    unsigned long frameTime = millis() - frameStart;
    unsigned long totalTime = millis() - startTime;
    float fps = (frameCount * 1000.0) / totalTime;

    Serial.printf("✅ Frame %lu: %d bytes inviati in %lu ms (FPS: %.1f)\n",
                  frameCount, fb->len, frameTime, fps);

    // Pausa tra i frame per controllare FPS
    delay(30); // ~30 FPS (era 50ms per ~20 FPS)

    // Controlla connessione
    if (!client.connected()) {
      Serial.printf("❌ Frame %lu: client disconnesso\n", frameCount);
      break;
    }
  }

  Serial.printf("🏁 Streaming terminato: %lu frame in %lu ms (%.1f FPS)\n",
                frameCount, millis() - startTime,
                (frameCount * 1000.0) / (millis() - startTime));
}

void setupServer() {
  Serial.println("🌐 Configurazione webserver...");

  server.on("/", handleRoot);
  server.on("/capture", handleCapture);
  server.on("/photo", handlePhoto);
  server.on("/stream", handleStream);     // Pagina streaming
  server.on("/video", handleVideoStream); // Stream MJPEG

  server.begin();
  Serial.println("✅ Webserver avviato sulla porta 80");
}

void loop() { server.handleClient(); }

} // namespace webserver.