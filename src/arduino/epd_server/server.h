#include "ESPAsyncWebServer.h"

AsyncWebServer server(80);
// TODO: Refactor into system settings or config files
#define SCREEN_SIZE_X 800
#define SCREEN_SIZE_Y 480
#define PAGE_STEPS 10
#define SCREEN_RESOLUTION SCREEN_SIZE_X*SCREEN_SIZE_Y/8   // Display size in pixels.
#define PAGE_INCREMENT_SIZE SCREEN_SIZE_Y/PAGE_STEPS      // Number of pixels per page, 48 = 10 pages on 480px display
#define PAGE_BYTE_SIZE SCREEN_RESOLUTION/PAGE_STEPS
byte imageByteArray[SCREEN_RESOLUTION]; // Target bayte array. Currently global, waste of ram
//byte *imageByteArray;
void showPicture(byte * imagePage, int page, int pagePixelIncrement);
void refreshDisplay();
void wipeDisplay();
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

byte *getImageBuffer() {
  return imageByteArray;
}

/**
 * Processes a POST request uplaoding an image. Is called on each arriving chunk, hence needs
 * to handle being called multiple times -> static vars
 */
void onScreen(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (!index) {
    Serial.printf("Processing... ");
    //imageByteArray = (byte *)malloc (SCREEN_RESOLUTION);
    if (!imageByteArray) {
      Serial.println("Malloc not successfull");
    }
  }
  //static byte imageByteArray[SCREEN_RESOLUTION];
  static String valueString = ""; // must be static to catch one byte breaking around a reqeust chunk
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
    byte *imagePage = imageByteArray;
    showPicture(imagePage, 0 , PAGE_INCREMENT_SIZE);
    //wipeDisplay();
    //free(imageByteArray);
    elem = 0;
    Serial.printf(" done: %uB\n", total);
  }
}

String getMemoryUsage() {
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
    request->send(200, "text/plain", getMemoryUsage());
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

//  server.on(
//  "/api/image/on-screen", HTTP_POST, [](AsyncWebServerRequest * request) {}, NULL,
//  []( AsyncWebServerRequest * request, String filename,
//      size_t index, uint8_t *data, size_t len, bool final )
//  {
//    String          path ;                              // Filename including "/"
//    static File     f ;                                 // File handle output file
//    char*           reply ;                             // Reply for webserver
//    static uint32_t t ;                                 // Timer for progress messages
//    uint32_t        t1 ;                                // For compare
//    static uint32_t totallength ;                       // Total file length
//    static size_t   lastindex ;                         // To test same index
//
//    if ( index == 0 )
//    {
//      path = String ( "/" ) + filename ;                // Form SPIFFS filename
//      SPIFFS.remove ( path ) ;                          // Remove old file
//      f = SPIFFS.open ( path, "w" ) ;                   // Create new file
//      t = millis() ;                                    // Start time
//      totallength = 0 ;                                 // Total file lengt still zero
//      lastindex = 0 ;                                   // Prepare test
//    }
//    t1 = millis() ;                                     // Current timestamp
//    // Yes, print progress
//    dbgprint ( "File upload %s, t = %d msec, len %d, index %d",
//               filename.c_str(), t1 - t, len, index ) ;
//    if ( len )                                          // Something to write?
//    {
//      if ( ( index != lastindex ) || ( index == 0 ) )   // New chunk?
//      {
//        f.write ( data, len ) ;                         // Yes, transfer to SPIFFS
//        totallength += len ;                            // Update stored length
//        lastindex = index ;                             // Remenber this part
//      }
//    }
//    if ( final )                                        // Was this last chunk?
//    {
//      f.close() ;                                       // Yes, clode the file
//      reply = dbgprint ( "File upload %s, %d bytes finished",
//                         filename.c_str(), totallength ) ;
//      request->send ( 200, "", reply ) ;
//    }
//  });
