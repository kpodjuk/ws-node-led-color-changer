#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <WebSocketsServer.h>
#include  "FastLED.h"
#include "ArduinoJson.h"

// ************ Function definitions ************
void rainbowWave(uint8_t, uint8_t);
void executeEveryLoop(void);

// ************ Global vars ************
bool rainbow = false; // The rainbow effect is turned off on startup
unsigned long start, end = 0;
unsigned long prevMillis = millis();
int hue = 0;

// solid color from json message
uint32_t solidColor = 0xFF0000; // default: red

enum OPERATING_MODE {
  ADALIGHT,
  SOLID_COLOR,
  RAINBOW,
  DIFFERENT_EFFECTS,
  BLINK

} currentOperatingMode;

#define NUM_LEDS 59
#define DATA_PIN 14 //D5 on board
#define JSON_MAXLENGTH 200

ESP8266WiFiMulti wifiMulti;       // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'

StaticJsonDocument<JSON_MAXLENGTH> jsonDoc;    // JSON document received with websocket

ESP8266WebServer server(80);       // Create a webserver object that listens for HTTP request on port 80
WebSocketsServer webSocket(81);    // create a websocket server on port 81

File fsUploadFile;                 // a File variable to temporarily store the received file


const char *ssid = "NodeMCU1"; // The name of the Wi-Fi network that will be created
const char *password = "";   // The password required to connect to it, leave blank for an open network

const char *OTAName = "ESP8266";           // A name and a password for the OTA service
const char *OTAPassword = "esp8266";

const char* mdnsName = "esp8266"; // Domain name for the mDNS responder

CRGB leds[NUM_LEDS];

void setup() {
  FastLED.addLeds<WS2813, DATA_PIN, GRB>(leds, NUM_LEDS); // Add led strip

  Serial.begin(115200);        // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println("\r\n");

  startWiFi();                 // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  
  startOTA();                  // Start the OTA service
  
  startSPIFFS();               // Start the SPIFFS and list all contents

  startWebSocket();            // Start a WebSocket server
  
  startMDNS();                 // Start the mDNS responder

  startServer();               // Start a HTTP server with a file read handler and an upload handler
  
}

void loop() {
  webSocket.loop();                           // constantly check for websocket events
  server.handleClient();                      // run the server
  ArduinoOTA.handle();                        // listen for OTA events

  // not a perfect solution but:
  // functions that have to be executed every loop will be executed here (rainbow for example)
  executeEveryLoop();
}


void startWiFi() { // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  WiFi.softAP(ssid, password);             // Start the access point
  Serial.print("Access Point \"");
  Serial.print(ssid);
  Serial.println("\" started\r\n");


  // will it connect to multiple networks or only one? 
  wifiMulti.addAP("Orange_Swiatlowod_Gora", "mlekogrzybowe");   // add Wi-Fi networks you want to connect to
  // wifiMulti.addAP("Orange_Swiatlowod_E8A0", "mlekogrzybowe);


  Serial.println("Connecting");
  while (wifiMulti.run() != WL_CONNECTED && WiFi.softAPgetStationNum() < 1) {  // Wait for the Wi-Fi to connect
    delay(250);
    Serial.print('.');
  }
  Serial.println("\r\n");
  if(WiFi.softAPgetStationNum() == 0) {      // If the ESP is connected to an AP
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID());             // Tell us what network we're connected to
    Serial.print("IP address:\t");
    Serial.print(WiFi.localIP());            // Send the IP address of the ESP8266 to the computer
  } else {                                   // If a station is connected to the ESP SoftAP
    Serial.print("Station connected to ESP8266 AP");
  }
  Serial.println("\r\n");
}

void startOTA() { // Start the OTA service
  ArduinoOTA.setHostname(OTAName);
  ArduinoOTA.setPassword(OTAPassword);

  ArduinoOTA.onStart([]() {
    Serial.println("Start");

  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\r\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA ready\r\n");
}

void startSPIFFS() { // Start the SPIFFS and list all contents
  SPIFFS.begin();                             // Start the SPI Flash File System (SPIFFS)
  Serial.println("SPIFFS started. Contents:");
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {                      // List the file system contents
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("\tFS File: %s, size: %s\r\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
  }
}

void startWebSocket() { // Start a WebSocket server
  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
  Serial.println("WebSocket server started.");
}

void startMDNS() { // Start the mDNS responder
  MDNS.begin(mdnsName);                        // start the multicast domain name server
  Serial.print("mDNS responder started: http://");
  Serial.print(mdnsName);
  Serial.println(".local");
}

void startServer() { // Start a HTTP server with a file read handler and an upload handler
  server.on("/edit.html",  HTTP_POST, []() {  // If a POST request is sent to the /edit.html address,
    server.send(200, "text/plain", ""); 
  });

  server.onNotFound(handleNotFound);          // if someone requests any other file or page, go to function 'handleNotFound'
                                              // and check if the file exists

  server.begin();                             // start the HTTP server
  Serial.println("HTTP server started.");
}

void handleNotFound(){ // if the requested file or page doesn't exist, return a 404 not found error
  if(!handleFileRead(server.uri())){          // check if the file exists in the flash memory (SPIFFS), if so, send it
    server.send(404, "text/plain", "404: File Not Found");
  }
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file-> ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { // When a WebSocket message is received
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        rainbow = false;                  // Turn rainbow off when a new connection is established
      }
      break;
    case WStype_TEXT:                     // if new text data is received
      Serial.printf("[%u] get Text: %s\n", num, payload);

      // String payload_str = String((char*) payload);

      // process received JSON
      DeserializationError error = deserializeJson(jsonDoc, payload);

      // Test if parsing succeeds.
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;  
      }

      // Fetch values.
      // Most of the time, you can rely on the implicit casts.
      // In other case, you can do doc["time"].as<long>();

      if(jsonDoc["type"] == "SOLID_COLOR"){ // SOLID_COLOR
          currentOperatingMode = SOLID_COLOR;
          solidColor = jsonDoc["color"];
          checkOperationMode();
      }
      else if(jsonDoc["type"] == "RAINBOW"){ // RAINBOW
          currentOperatingMode = RAINBOW;
          checkOperationMode();
      } 
      else if(jsonDoc["type"] == "BLINK"){ // Other operating modes go here
          currentOperatingMode = BLINK;
          checkOperationMode();

      } 
      else if(jsonDoc["type"] == "STATUS_UPDATE_NEEDED"){ // Client needs status update
        sendStatus();
      }




      break;
  }
}

String formatBytes(size_t bytes) { // convert sizes in bytes to KB and MB
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  }
}

String getContentType(String filename) { // determine the filetype of a given filename, based on the extension
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

void sendStatus(){
  // send current status to websocket client here (mode, settings for that mode)
  jsonDoc["type"] = "STATUS_UPDATE";
  jsonDoc["OPERATING_MODE"] = currentOperatingMode; 

  if(currentOperatingMode == SOLID_COLOR){ // attach info about choosen setting if needed
  jsonDoc["solidColor"] = solidColor;
  }

  String statusString;
  serializeJson(jsonDoc, statusString);


  // JsonArray data = doc.createNestedArray("data");
  // data.add(48.756080);
  // data.add(2.302038);

  // It's sent to every client connected, not the one who requested it
  // no harm in that tho
  webSocket.broadcastTXT(statusString);

}


void setHue(int hue) { // Set the RGB LED to a given hue (color) (0° = Red, 120° = Green, 240° = Blue)
  hue %= 360;                   // hue is an angle between 0 and 359°
  float radH = hue*3.142/180;   // Convert degrees to radians
  float rf, gf, bf;
  
  if(hue>=0 && hue<120){        // Convert from HSI color space to RGB              
    rf = cos(radH*3/4);
    gf = sin(radH*3/4);
    bf = 0;
  } else if(hue>=120 && hue<240){
    radH -= 2.09439;
    gf = cos(radH*3/4);
    bf = sin(radH*3/4);
    rf = 0;
  } else if(hue>=240 && hue<360){
    radH -= 4.188787;
    bf = cos(radH*3/4);
    rf = sin(radH*3/4);
    gf = 0;
  }
  int r = rf*rf*1023;
  int g = gf*gf*1023;
  int b = bf*bf*1023;

          r = r/4;
        g = g/4;
        b = b/4;
  
        for(int i = 0; i<NUM_LEDS; i++){
        leds[i].red = (uint8_t)r;                          // write it to the LED output pins
        leds[i].green = (uint8_t)g;
        leds[i].blue = (uint8_t)b;
        }


}
void rainbowWave(uint8_t thisSpeed, uint8_t deltaHue) {     // The fill_rainbow call doesn't support brightness levels.
 
// uint8_t thisHue = beatsin8(thisSpeed,0,255);                // A simple rainbow wave.
 uint8_t thisHue = beat8(thisSpeed,255);                     // A simple rainbow march.
  
 fill_rainbow(leds, NUM_LEDS, thisHue, deltaHue);            // Use FastLED's fill_rainbow routine.
 
} 


void checkOperationMode(void){

  if(currentOperatingMode == SOLID_COLOR){ // set whole strip to one color
    // set color here, solidColor global var contains color info
        
    for(int i = 0; i<NUM_LEDS; i++){
      leds[i] = solidColor; // write it to the LED output pins                    
    }
    FastLED.show();


  } else if (currentOperatingMode == RAINBOW){
    // Serial.print("RAINBOW!\n");
    // rainbow function has to be executed in loop: rainbowWave(200, 10)
    // should write effect functions that don't require that

  } else if (currentOperatingMode == BLINK){

  }


}


void executeEveryLoop(){
  
  // rainbow
  if(currentOperatingMode == RAINBOW){
    rainbowWave(200, 10);
    FastLED.show();
  } else if (currentOperatingMode == BLINK){
    doBlinkStep();
  }
  
}

void doBlinkStep(){
  static bool is_red = false;
  int actTime = millis();
  static int remTime;
  const int period = 500;
  if (actTime - remTime >= period){
    remTime = actTime;
    if(!is_red) {
        is_red = true;
        fill_solid(leds, NUM_LEDS, CRGB::Red);
        FastLED.show();
    } else {
        is_red = false;
        fill_solid(leds, NUM_LEDS, CRGB::Black);
        FastLED.show();
    }
  }
}

