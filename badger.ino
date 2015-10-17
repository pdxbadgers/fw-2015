/*
 * Copyright (c) 2015, Majenko Technologies
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * * Neither the name of Majenko Technologies nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Do Badger networking things */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <stdint.h>

//something funny about arduino is that it seems to parse your .ino
//file for #include to see what to set the include path to
#include <EEPROM.h>
#include "configspace.h"

#include <Ticker.h>
#include "leds.h"

#define TAIL_LED       0  //led 0
#define BACK_FOOT_LED  16 //led 1
#define FRONT_FOOT_LED 14 //led 2
#define NOSE_LED       5  //led 3

#define EYE_R          13
#define EYE_G          15
#define EYE_B          12

MonochromeLED tailLED(TAIL_LED);
MonochromeLED backFootLED(BACK_FOOT_LED);
MonochromeLED frontFootLED(FRONT_FOOT_LED);
MonochromeLED noseLED(NOSE_LED);
RgbLED eyeLED(EYE_R, EYE_G, EYE_B);

LED* _led_array[] = { &tailLED, &backFootLED, &frontFootLED, &noseLED, &eyeLED };
#define NUM_LEDS ( sizeof(_led_array) / sizeof(LED*) )

LEDGroup leds(_led_array, NUM_LEDS, [](){ leds.modeStep(); });

/* Set these to your desired credentials. */
#define PREFERRED_INFRASTRUCTURE_SSID "BadgerNet"
#define WIFI_HOTSPOT_BASENAME         "BadgerNet"
#define WIFI_CONNECT_RETRIES          4

String myhostname = "none";
ESP8266WebServer server(80);

#define DEFAULT_MODE_CHANGE_TIME  (20*1000)
#define DEFAULT_MODE_PAUSE_TIME   (5*1000)

String flag = "BADGERMASTER";

const char serverIndex[] = "<html><head><style>body {text-align:center;} h2 {border-style: solid none;} .footer {background-color:orange;}</style>"
                                       "<title>BSides PDX 2015 Badger</title><head>"
                                 "<body><h2>BSides PDX 2015 Badger</h2>"
                                 "<div class='buttons'>"
                                   "<div><a href='/leds?m=none'>Off</a></div>"
                                   "<div><a href='/leds?m=all'>All On</a></div>"
                                   "<div><a href='/leds?m=twinkle'>Random</a></div>"
                                 "</div>"
                                 "<div class='footer'>See <a href='https://github.com/pdxbadgers/fw-2015'>GitHub</a> for all features.</div>"
                           "</body></html>";

void handleRoot()
{
  Serial.println("Root, there it is!");
  String html(serverIndex);
  server.send(200, "text/html", html);
}

void handleFlag() {
  if(server.hasArg("newflag")) {
    flag = server.arg("newflag");

    // Put the badger in notification mode that a flag has been planted
    defaultModePause();
    leds[4]->setColor(0, 0, 0);
    leds[4]->setState(true);
  }

  server.send(200, "text/html", flag);
}

Ticker defaultModeTicker;
boolean defaultModePaused = false;

void defaultModeStart() {
  Serial.println("defaultModeStart");
  defaultModePaused = false;
  leds.setMode((random(0,1) == 0) ? "chase" : "twinkle");

  defaultModeTicker.once_ms(DEFAULT_MODE_CHANGE_TIME, defaultModeStart);
}

void defaultModePause() {
  Serial.println("defaultModePause");
  if (!defaultModePaused) {
    leds.setMode("none");
  }
  defaultModePaused = true;
  defaultModeTicker.once_ms(DEFAULT_MODE_PAUSE_TIME, defaultModeStart);
}

void defaultModeStop() {
  Serial.println("defaultModeStop");
  defaultModeTicker.detach();
}

// GET /leds -> JSON describing LED set
// GET /leds/ -> JSON describing all LEDs
// GET /leds?d=<id> -> equiv GET /leds/<id>
// GET /leds?d=<id>&s=<state> -> equiv PATCH /leds/<id> {"state": <state>}
// GET /leds?d=<id>&r=<red>&g=<green>&b=<blue> -> equiv PATCH /leds/<id> {"r": <red>, "g": <green>, "b": <blue>}
// GET /leds?d=<id>&s=<state>&r=<red>&g=<green>&b=<blue> -> equiv PUT /leds/<id> {"state": <state>, "r": <red>, "g": <green>, "b": <blue>}
// GET /leds?m=<mode> -> equiv PATCH /leds {"mode": <mode>}
//    <mode> is one of:
//      blink: all LEDs flash
//      chase: two LEDs will chase across the badger
//      twinkle: random, overlapping blink
//      none: all LEDs off
void handleLeds() {
  Serial.println("handleLeds()");
  String uri = server.uri();

  if (uri[uri.length() - 1] == '/') {
    // Enumerate LEDs
    if (server.args() != 0) {
      sendError(400, "too many parameters");
      return;
    }

    server.send(200, "text/html", leds.getGroupEnumeration());
  }
  else if (server.args() == 0) {
    //LEDs collection info
    server.send(200, "text/html", leds.getGroupInfo());
  }
  else if (server.hasArg("m")) {
    if (server.args() > 1) {
      sendError(400, "setting mode has no additional parameters");
      return;      
    }

    defaultModeStop();

    if (!leds.setMode(server.arg("m"))) {
      sendError(400, "unknown mode");
      return;
    }

    server.send(200, "text/html", leds.getGroupInfo());
  }
  else if (server.hasArg("d")) {
    Serial.println("d=");
    int expectedParams = 1;

    int ledID = server.arg("d").toInt();
    if (ledID < 0 || ledID >= NUM_LEDS) {
      sendError(404, "no LED with requested id");
      return;
    }
    Serial.print("device id: ");
    Serial.println(ledID);
    
    boolean setNewState = false;
    boolean newState = false;
    if (server.hasArg("s")) {
      Serial.println("s=");
      expectedParams++;
      setNewState = true;
      newState = (server.arg("s").toInt() != 0);
      Serial.print("newState: ");
      Serial.println(newState);
    }
    
    boolean setRgb = false;
    int newRed = 0, newGreen = 0, newBlue = 0;
    if (server.hasArg("r") && server.hasArg("g") && server.hasArg("b")) {
      Serial.println("rgb=");
      if (!leds[ledID]->isRgb()) {
        sendError(400, "requested LED is not color selectable");
        return;
      }
      expectedParams += 3;
      setRgb = true;
      newRed = server.arg("r").toInt();
      newGreen = server.arg("g").toInt();
      newBlue = server.arg("b").toInt();
      Serial.println("setting color");
    }

    if (server.args() != expectedParams) {
      Serial.println("param count mismatch");
      sendError(400, "unknown params");
      return;
    }

    defaultModeStop();

    if (setRgb && !leds[ledID]->setColor(newRed, newGreen, newBlue)) {
      Serial.println("error setting color");
      sendError(400, "RGB set failed");
      return;
    }
    if (setNewState) {
      leds[ledID]->setState(newState);
    }
    server.send(200, "text/html", leds.getLedInfo(ledID));
  }
  else {
    Serial.println("unknown led action");
    sendError(404, "unknown action");
  }
}

void handleConfig()
{
  if (server.hasArg("n")) {
    //set the name
    const String name = server.arg("n");
    configSpace.setName(server.arg("n"));
  }
  if (server.hasArg("s")) {
    //set speaker
    bool value = false;
    if (server.arg("s") == "1") {
      value = true;
    }
    configSpace.setSpeaker(value);
  }

  String content("{\"s\":");
  content.reserve(3+configSpace.nameLen()+1);
  if (configSpace.speaker()) {
    content += "1";
  } else {
    content += "0";
  }
  content += ",\"n\":\"";
  {
    //delete returned string prior to sending it with server
    content += configSpace.name();
  }
  content += "\"}";
  server.send(200, "text/html", content);
}

void setupWiFi() {
  Serial.print("MAC address is ");
  Serial.println(WiFi.macAddress());

  String macLastFour = WiFi.macAddress().substring(12,17);
  macLastFour.replace(":","");
  macLastFour.toLowerCase();

  myhostname = configSpace.name();
  if (myhostname[0] == 0xff) { // this is what EEPROM API returns if not initialized
    Serial.print("Found uninitialized config! Setting default");
    myhostname = "";
    configSpace.setName(myhostname);
  }
  Serial.print("name from config is ");
  Serial.println(myhostname);
  if (myhostname.length() == 0 || (myhostname.length() == 1 && myhostname[0] == 0)) {
    myhostname = "badger";
    myhostname += "-";
    myhostname += macLastFour;
  }
  Serial.print("Hostname is ");
  Serial.println(myhostname);
  WiFi.hostname(myhostname);

  Serial.println("Configuring access point...");
  bool online=0;

  Serial.println("Trying to connect to " PREFERRED_INFRASTRUCTURE_SSID);
  leds.setAllState(false);
  WiFi.begin(PREFERRED_INFRASTRUCTURE_SSID);
  int result = WL_DISCONNECTED;
  for (int i = 0; i < WIFI_CONNECT_RETRIES && result != WL_CONNECTED; i++) {
    Serial.print("Attempt ");
    Serial.print(i + 1);
    Serial.print(" of ");
    Serial.println(WIFI_CONNECT_RETRIES);
    leds[i]->setState(true);

    result = WiFi.waitForConnectResult();
    if (result != WL_CONNECTED) {
      delay(500);
    }
  }
  leds.setAllState(false);
  if(result == WL_CONNECTED) {
    online = 1;
    Serial.println("Connected to " PREFERRED_INFRASTRUCTURE_SSID "!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Set external indicator that we're in config mode
    eyeLED.setColor(COLOR_GREEN);
    eyeLED.setState(true);
  }
  else {
    Serial.println("Couldn't find " PREFERRED_INFRASTRUCTURE_SSID "!");
  }

  if(!online) { // I'll make my own AP!
    String ssid(WIFI_HOTSPOT_BASENAME "-");
    ssid += macLastFour;

    Serial.print(F("Fine, I'll make my own AP at "));
    Serial.println(ssid);
    WiFi.softAP(ssid.c_str());
    online=1;
    Serial.print(F("AP IP address: "));
    Serial.println(WiFi.softAPIP());

    // Set external indicator that we're in config mode
    eyeLED.setColor(COLOR_RED);
    eyeLED.setState(true);
  }
  else {
    // MDNS only works when we are not the AP host

    // Add service to MDNS-SD
    MDNS.addService("http", "tcp", 80);
    if (!MDNS.begin(myhostname.c_str())) {
      Serial.println("Error setting up MDNS responder!");
      Serial.println("I guess I'll need to be found by IP address.");
    }
  }

  // Wait a few seconds and turn off the light, because it's really bright
  delay(3000);
  leds.setAllState(false);
  eyeLED.setColor(COLOR_RANDOM);
}

void handleFileUpload()
{
  if(server.uri() != "/update") return;
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START)
    {
      Serial.setDebugOutput(true);
      WiFiUDP::stopAll();
      Serial.printf("Update: %s\n", upload.filename.c_str());
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if(!Update.begin(maxSketchSpace))
        {//start with max available size
          Update.printError(Serial);
        }
    }
  else if(upload.status == UPLOAD_FILE_WRITE)
    {
      if(Update.write(upload.buf, upload.currentSize) != upload.currentSize)
        {
          Update.printError(Serial);
        }
    }
  else if(upload.status == UPLOAD_FILE_END)
    {
      if(Update.end(true))
        { //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        }
      else
        {
          Update.printError(Serial);
        }
      Serial.setDebugOutput(false);
    }
  yield();
}

void handleUpdate() {
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
  ESP.restart();
}

void sendError(int code, const String& content) {
  server.send(code, "text/plain", content);  
}

void setup()
{
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  leds.setup();

  configSpace.begin();

  setupWiFi();

  server.on("/", handleRoot);
  server.on("/flag", handleFlag);
  server.on("/leds", handleLeds);
  server.on("/leds/", handleLeds);  
  server.on("/config", handleConfig);

  server.on("/update", HTTP_POST, handleUpdate);
  server.onFileUpload(handleFileUpload);

  server.begin();
  Serial.println(F("HTTP server started"));

  defaultModeStart();
}

void loop() {
  server.handleClient();
}

/*
 * Editor modelines  -  https://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * mode: c++
 * c-basic-offset: 2
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=2 tabstop=8 expandtab:
 * :indentSize=2:tabSize=8:noTabs=true:
 */
