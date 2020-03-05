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
  * TODO: Replace with handler for all root requests
  */
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/www/index.html", "text/html");
  });
  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/www/index.html", "text/html");
  });
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/www/favicon.ico", "text/html");
  });
  
  server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/styles.css.gz", "text/css");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  server.on("/runtime.js", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/runtime.js.gz", "application/javascript");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  server.on("/polyfills.js", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/polyfills.js.gz", "application/javascript");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  server.on("/main.js", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/main.js.gz", "application/javascript");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

 
  server.begin();
}
 
void loop(){}
