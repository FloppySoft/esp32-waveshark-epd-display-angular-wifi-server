
#include "WiFi.h"
#include "SPIFFS.h"
#include "ESPAsyncWebServer.h"

#include "local_dev_config.h" // optional local dev settings, see readme

/* 
 * Remember not to commit your credentials on github :P 
 * Using ignored file instead
 */
#ifndef wifiSettings
const char* ssid = "My Wifi Network";
const char* password = "My totally secure password";
#endif

AsyncWebServer server(80);
 
void setup(){
  Serial.begin(115200);
 
  if(!SPIFFS.begin()){
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
  }
 
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
 
  Serial.println(WiFi.localIP());

 /*
  * Hardcoded requests for all needed angular files.
  * TODO: Utilize lib capabitilies, e.g. wildcards or regex to serve any file.
  */
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });
    server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });
      server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/styles.css", "text/css");
  });
        server.on("/runtime.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/runtime.js", "application/javascript");
  });
          server.on("/polyfills.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/polyfills.js", "application/javascript");
  });
          server.on("/main.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/main.js", "application/javascript");
  });
          server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/favicon.ico", "text/html");
  });
 
  server.begin();
}
 
void loop(){}
