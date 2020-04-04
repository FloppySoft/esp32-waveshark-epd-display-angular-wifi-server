#include <Arduino.h>
#include "WiFi.h"
#include "FS.h"
#include "SPIFFS.h"
#include "ESPAsyncWebServer.h"
#include <GxEPD2_BW.h>

// == CONFIG ==
/*
Remember not to commit your credentials on github :P
Using ignored file instead
*/
#include "local_dev_config.h" // optional local dev settings, see readme
#ifndef wifiSettings
const char *ssid = "My Wifi Network";
const char *password = "My totally secure password";
#endif

// TODO: Refactor into system settings or config files
#define SCREEN_SIZE_X 800
#define SCREEN_SIZE_Y 480
#define PAGE_STEPS 16
#define SELECTED_IMAGE_PATH "/image.bin"


/*
GxEPD2 setup.
Page size is set by division into HEIGHT-pieces, e.g. 16 -> GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT / 16>
*/
//#define ENABLE_GxEPD2_GFX 0
GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT / PAGE_STEPS> display(GxEPD2_750_T7(/*CS=*/15, /*DC=*/27, /*RST=*/26, /*BUSY=*/25));

AsyncWebServer webServer(80);


static uint8_t *framebuffer;
bool isImageRefreshPending = false;
bool isDisplayJobScheduled = false;
unsigned long displayJobStart = 0;

void clearDisplay()
{
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
  } while (display.nextPage());
}

void showFile(fs::FS &fs, const char *path)
{
  Serial.printf("Reading file: %s\r\n", path);
  /*
    So why dynamically allocating memory here instead of a simple declaration:
    unsigned char image[48000]
    The ESP32 has a flaw allowing to sue only 160KB of static ram.
    See https://github.com/espressif/esp-idf/issues/3497
    You can be lucky and be warned during compilation, but often the compiler
    does not recognize this limit and espiecially with our large image buffer, 
    this let's the ESP32 crash during runtime. The compiler not warning you doesn't mean it will work.
    The only way around above 160KB is using ram dynamically done as follows.
    */
  framebuffer = (uint8_t *)calloc(48000, sizeof(uint8_t));
  assert(framebuffer != NULL);
  File file = fs.open(path);
  if (!file || file.isDirectory())
  {
    Serial.println("- failed to open file for reading");
    return;
  }
  long element = 0;
  Serial.println("- read from file:");
  while (file.available())
  {
    framebuffer[element] = file.read();
    element++;
  }
  file.close();
  display.drawImage(framebuffer, 0, 0, 800, 480, true, false, true);
  free(framebuffer);
}

void showSelectedImage()
{
  showFile(SPIFFS, SELECTED_IMAGE_PATH);
}

/**
 * Handles API request to show selected image.
 * Does not directly call display methods because of strict server timeouts.
 * Currently, threading is not wanted, hence setting a global flag for polling.
 */
String handleSelectedImageRefresh()
{
  isImageRefreshPending = true;
  return ("OK");
}

String getMemoryUsage()
{
  String usage = String(ESP.getFreeHeap());
  return usage;
}

String getFullMemoryUsage()
{
  String usage = "Total heap: ";
  usage = String(usage + ESP.getHeapSize());
  usage = String(usage + "\nFree heap: ");
  usage = String(usage + String(ESP.getFreeHeap()));
  usage = String(usage + "\nTotal PSRAM: ");
  usage = String(usage + String(ESP.getPsramSize()));
  usage = String(usage + "\nFree PSRAM: ");
  usage = String(usage + String(ESP.getFreePsram()));
  return usage;
}

/**
 * Handles multipart file upload and save the file on spiffs.
 * Modified from source: https://github.com/Edzelf/Esp-radio
 * MIT License
 * Copyright (c) 2019 Ed Smallenburg
 **/
void handleSingleFileUpload(AsyncWebServerRequest *request, String filename,
                      size_t index, uint8_t *data, size_t len, bool final)
{
  Serial.println("upload handle starting.");
  String path;
  static File f;
  static uint32_t totallength;
  static size_t lastindex;
  if (index == 0)
  {
    path = String("/") + filename;
    SPIFFS.remove(path);                        // Delete old file
    f = SPIFFS.open(path, "w");                 // Create new file
    totallength = 0;
    lastindex = 0;
  }
  if (len) // Something to write?
  {
    if ((index != lastindex) || (index == 0))   // New chunk?
    {
      f.write(data, len);
      totallength += len;
      lastindex = index;
    }
  }
  if (final)
  {
    f.close();
    request->send(200, "OK");
    isImageRefreshPending = true;
  }
}

/**
 * Sets up all REST endpoints and their responses.
 * Angular artifacts are treated specially here to allow pre-compression.
 */
void startServer()
{
  /*
  Server reqeusts for Angular artifacts
  Hardcoded requests for all requested angular files.
  TODO: Replace with prezip-handler for all root file requests
  */
  webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/www/index.html", "text/html");
  });
  webServer.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/www/index.html", "text/html");
  });
  webServer.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/www/favicon.ico", "text/html");
  });
  webServer.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/styles.css.gz", "text/css");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  webServer.on("/runtime.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/runtime.js.gz", "application/javascript");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  webServer.on("/polyfills.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/polyfills.js.gz", "application/javascript");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  webServer.on("/main.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/main.js.gz", "application/javascript");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  /*
  API: System
  */
  webServer.on("/api/system/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", getMemoryUsage());
  });

  /*
  API: Image
  */
  webServer.on("/api/image/upload-single", HTTP_POST, [](AsyncWebServerRequest *request) {
    //request->send(200);
  },
               handleSingleFileUpload);

  webServer.on("/api/image/show-selected", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", handleSelectedImageRefresh());
  });

  webServer.begin();
}

/**
 * Initializes Wifi, but cycles it twice to avoid the common problem
 * of stuck wifi on startup. Waiting times are random/empirically set up,
 * hence can be optimized.
 */
void initWifi()
{
  WiFi.begin(ssid, password);
  delay(1000);
  WiFi.mode(WIFI_OFF);
  delay(500);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  delay(100);
  Serial.println(WiFi.localIP());
}

void initDisplaySpi()
{
  display.init(0); // 0 avoids this lib to spawn it's own Serial object
  // uses standard SPI pins, e.g. SCK(18), MISO(19), MOSI(23), SS(5)
  // *** special handling for Waveshare ESP32 Driver board *** //
  // ********************************************************* //
  SPI.end(); // release standard SPI pins, e.g. SCK(18), MISO(19), MOSI(23), SS(5)
  //SPI: void begin(int8_t sck=-1, int8_t miso=-1, int8_t mosi=-1, int8_t ss=-1);
  SPI.begin(13, 12, 14, 15); // map and init SPI pins SCK(13), MISO(12), MOSI(14), SS(15)
  // *** end of special handling for Waveshare ESP32 Driver board *** //
  // **************************************************************** //
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root)
  {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory())
  {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels)
      {
        listDir(fs, file.name(), levels - 1);
      }
    }
    else
    {
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

void drawTestPicture()
{
  display.setFullWindow();
  display.clearScreen(); // use default for white

  //display.drawImage(picture, 0, 0, 800, 480, true, false, true);
}

void handleDisplayJob()
{
  if (isImageRefreshPending)
  {
    displayJobStart = millis() + 1000UL;
    isImageRefreshPending = false;
    isDisplayJobScheduled = true;
  }
  if (isDisplayJobScheduled && millis() > displayJobStart)
  {
    isDisplayJobScheduled = false;
    clearDisplay();
    showSelectedImage();
  }
}

void setup()
{
  // Display init must be before Serial for some reason.
  initDisplaySpi();
  Serial.begin(115200);
  if (!SPIFFS.begin())
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  initWifi();
  startServer();
  listFiles();
  //drawTestPicture();
  //showSelectedImage();
}

void loop()
{
  handleDisplayJob();
}
