#include "WiFi.h"
#include "FS.h"
#include "SPIFFS.h"

#include "server.h"

//#include <GxEPD.h>
//#include <GxGDEW075T7/GxGDEW075T7.h>
//#include <GxIO/GxIO_SPI/GxIO_SPI.h>
//#include <GxIO/GxIO.h>
//GxIO_Class io(SPI, /*CS=5*/ 15, /*DC=*/ 27, /*RST=*/ 27); // Pins of Waveshark ESP32 display driver
//GxEPD_Class display(io, /*RST=*/ 27, /*BUSY=*/ 25);

// For GxEPD2 - currently not working, overflow?
// #define ENABLE_GxEPD2_GFX 0
 #include <GxEPD2_BW.h>
 #define MAX_DISPLAY_BUFFER_SIZE 15000ul // ~15k is a good compromise
 #define MAX_HEIGHT(EPD) (EPD::HEIGHT <= MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8) ? EPD::HEIGHT : MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8))
 GxEPD2_BW<GxEPD2_750_T7, MAX_HEIGHT(GxEPD2_750_T7)> display(GxEPD2_750_T7(/*CS=4*/ 15, /*DC=*/ 27, /*RST=*/ 26, /*BUSY=*/ 25)); // GDEW075T7 800x480
 //GxEPD2_BW < GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT / 2 > display(GxEPD2_750_T7(/*CS=D8*/ 15, /*DC=D3*/ 27, /*RST=D4*/ 26, /*BUSY=D2*/ 25)); // GDEW075T7 800x480


// == CONFIG ==
/*
   Remember not to commit your credentials on github :P
   Using ignored file instead
*/
#include "local_dev_config.h" // optional local dev settings, see readme
#ifndef wifiSettings
const char* ssid = "My Wifi Network";
const char* password = "My totally secure password";
#endif


void setup() {
  Serial.begin(115200);

  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  startWifi();
  startServer();
  initDisplaySpi();
  listFiles();
  display.drawPaged(clearCallback);
  //display.fillScreen(GxEPD_WHITE);
}

void loop() {}

void startWifi() {
//  WiFi.begin(ssid, password);
//  delay(500);
//  WiFi.mode( WIFI_OFF );
//  delay(500);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  delay(100);
  Serial.println(WiFi.localIP());
}

void showPicture(byte * imagePage, int page, int pagePixelIncrement) {
  display.drawPaged(drawCallback);
}

void clearCallback() {
  display.fillScreen(GxEPD_WHITE);
}

void drawCallback() {
  display.fillScreen(GxEPD_WHITE);
  display.drawExampleBitmap(getImageBuffer(), sizeof(getImageBuffer()));
}

// == Functions ==
void initDisplay_old() {
  display.init(115200); // enable diagnostic output on Serial
  display.fillScreen(GxEPD_BLACK);
  display.fillScreen(GxEPD_WHITE);
}

void initDisplaySpi() {
  display.init(115200); // uses standard SPI pins, e.g. SCK(18), MISO(19), MOSI(23), SS(5)
  // *** special handling for Waveshare ESP32 Driver board *** //
  // ********************************************************* //
  SPI.end(); // release standard SPI pins, e.g. SCK(18), MISO(19), MOSI(23), SS(5)
  //SPI: void begin(int8_t sck=-1, int8_t miso=-1, int8_t mosi=-1, int8_t ss=-1);
  SPI.begin(13, 12, 14, 15); // map and init SPI pins SCK(13), MISO(12), MOSI(14), SS(15)
  // *** end of special handling for Waveshare ESP32 Driver board *** //
  // **************************************************************** //
  delay(500);
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void listFiles()
{
  listDir(SPIFFS, "/", 0);
}
