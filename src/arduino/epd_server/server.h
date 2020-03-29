#include "ESPAsyncWebServer.h"
AsyncWebServer server(80);
// TODO: Refactor into system settings or config files
#define SCREEN_RESOLUTION 48000 // Display size in pixels.
byte imageByteArray[SCREEN_RESOLUTION]; // Target bayte array. Currently global, waste of ram

void handleUpload(AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index) {
    Serial.printf("UploadStart: %s\n", filename.c_str());
  }
  for (size_t i = 0; i < len; i++) {
    Serial.print(data[i]);
  }
  if (final) {
    Serial.printf("UploadEnd: %s, %u B\n", filename.c_str(), index + len);
  }
}

void onScreen(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (!index) {
    Serial.printf("Processing... ");
  }
  String valueString = "";
  static long elem = 0; // Target iterator, static to survive each chun
  for (size_t i = 0; i < len; i++) {
    byte byteItem = data[i];
    if (byteItem == 44) {  // Skip ','
      imageByteArray[elem] = valueString.toInt();
      //Serial.printf("---> Saving byte value %u at elem= %u \n", valueString.toInt(), elem);
      valueString = "";
      if (elem > SCREEN_RESOLUTION) {
        Serial.println("");
        Serial.println("WARNING Data is larger than array. Skipping...");
        elem = 0;
      }
      elem++;
      continue;
    }
    if (isDigit(byteItem)) {
      valueString += (char)byteItem;
    }
  }
  if ((index + len) == total) {
    if (elem != (SCREEN_RESOLUTION - 1)) {
      Serial.println("");
      Serial.println("WARNING Data is larger than array. Skipping...");
    }
    elem = 0;
    Serial.printf(" done: %uB\n", total);
  }
}

void startServer() {
  /*
     Hardcoded requests for all needed angular files.
     TODO: Replace with handler for all root requests
  */
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/www/index.html", "text/html");
  });
  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/www/index.html", "text/html");
  });
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/www/favicon.ico", "text/html");
  });

  server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/styles.css.gz", "text/css");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  server.on("/runtime.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/runtime.js.gz", "application/javascript");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  server.on("/polyfills.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/polyfills.js.gz", "application/javascript");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  server.on("/main.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/main.js.gz", "application/javascript");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  /**
     Get free memory
  */
  server.on("/api/system/heap", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  /**
     Post byteArray of pixel vallues and put directly on screen
  */
  server.on(
  "/api/image/on-screen", HTTP_POST, [](AsyncWebServerRequest * request) {
    request->send(200);
  }, NULL, onScreen);

  server.begin();
}
