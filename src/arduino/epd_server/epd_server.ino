#include "WiFi.h"
#include "SPIFFS.h"

#include "server.h"
#include "local_dev_config.h" // optional local dev settings, see readme

/* 
 * Remember not to commit your credentials on github :P 
 * Using ignored file instead
 */
#ifndef wifiSettings
const char* ssid = "My Wifi Network";
const char* password = "My totally secure password";
#endif
 
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
  startServer();

}
 
void loop(){}
