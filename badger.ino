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

#define TAIL_LED       0  //led 0
#define BACK_FOOT_LED  16 //led 1
#define FRONT_FOOT_LED 14 //led 2
#define NOSE_LED       5  //led 3

#define EYE_R          13
#define EYE_G          15
#define EYE_B          12

/* Set these to your desired credentials. */
char ssid[60] = "BadgerNet";
ESP8266WebServer server(80);

String flag="BADGERMASTER";
String myhostname="none";

const char* serverIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";

void handleRoot()
{
  String html = "<h1>You are connected</h1>";
  html =+ serverIndex;
  server.send(200, "text/html", html);
}

void handleFlag()
{
  if(server.hasArg("newflag"))
    {
      flag=server.arg("newflag");
    }

  server.send(200, "text/html",flag);
}

void handleRGB()
{
  //format is
  //rgb?r={value}&g={value}&b={value}
  static uint8_t rgb[3];
  if (server.hasArg("r") && server.hasArg("g") && server.hasArg("b")) {
    //write
    rgb[0] = server.arg("r").toInt();
    rgb[1] = server.arg("g").toInt();
    rgb[2] = server.arg("b").toInt();
    analogWrite(EYE_R, rgb[0]);
    analogWrite(EYE_G, rgb[1]);
    analogWrite(EYE_B, rgb[2]);
  }
  String response;
  response += rgb[0];
  response += ", ";
  response += rgb[1];
  response += ", ";
  response += rgb[2];

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/html", response);
}

void handleBlink()
{
  //format is
  //blink?d={0,1,2,3}&v={value}
  //blink?d={0,1,2,3}
  static bool values[4];

  if (!server.hasArg("d")) {
    server.send(400, "text/html", "d={1..3}");
    return;
  }

  int led = (server.arg("d")).toInt();
  if (led < 0 || led > 3) {
    server.send(400, "text/html", "d={1..3}");
    return;
  }

  if (server.hasArg("v")) {
    //is a write
    int value = HIGH;

    if (server.arg("v") == "0") {
      value = LOW;
    }

    values[led] = value;
    switch(led) {
    case 0:
      digitalWrite(TAIL_LED, value);
      break;

    case 1:
      digitalWrite(BACK_FOOT_LED, value);
      break;

    case 2:
      digitalWrite(FRONT_FOOT_LED, value);
      break;

    case 3:
      digitalWrite(NOSE_LED, value);
      break;

    default:
      break;
    }
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/html", values[led] ? "1" : "0");
}

void setupWiFi()
{
  myhostname=WiFi.macAddress();
  myhostname.replace(":","");
  myhostname.toLowerCase();
  WiFi.hostname(myhostname);

  Serial.println("Configuring access point...");
  bool online=0;
  Serial.println("Trying to connect to BadgerNet");
  WiFi.begin("BadgerNet");
  if(WiFi.waitForConnectResult()==WL_CONNECTED)
    {
      online=1;
      Serial.println("Connected to BadgerNet!");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
    }
  else
    {
      Serial.println("Couldn't find BadgerNet!");
    }

  if(WiFi.status()!=WL_CONNECTED) // uh oh .. BadgerNet isn't there, is someone else running one?
    {
      Serial.println(F("Looking for more badgers.."));
      int n = WiFi.scanNetworks();
      for (int i = 0; i < n; ++i)
        {
          char this_ssid[60];
          String string_ssid=WiFi.SSID(i);
          string_ssid.toCharArray(this_ssid,60);
          if(string_ssid.startsWith("BadgerNet"))
            {
              Serial.print(F("Trying to connect to "));
              Serial.println(this_ssid);
              WiFi.begin(this_ssid);
              if(WiFi.waitForConnectResult()==WL_CONNECTED)
                {
                  online=1;
                  Serial.print(F("Connected to "));
                  Serial.println(this_ssid);

                  Serial.print(F("IP address: "));
                  Serial.println(WiFi.localIP());
                  break; // break out of the for loop
                }
              else
                {
                  Serial.print(F("Could not connect to "));
                  Serial.println(this_ssid);
                }
            }
          else
            {
              Serial.print(F("I don't care about "));
              Serial.println(WiFi.SSID(i));
            }
          delay(10);
        }
    }

  if(!online) // screw it, I'll make my own AP!
    {
      String postfix = WiFi.macAddress();
      //Serial.println(postfix);
      //Serial.println(postfix.substring(9,17));

      char macaddr[60];
      ("-"+WiFi.macAddress().substring(9,17)).toCharArray(macaddr,60);
      WiFi.softAP(strcat(ssid,macaddr));

      if(1) // best check ever
        {
          online=1;
          Serial.print(F("Fine, I'll make my own AP at "));
          Serial.println(ssid);
          Serial.print(F("AP IP address: "));
          Serial.println(WiFi.softAPIP());
        }
      else
        {
          Serial.println(F("Couldn't even set up my own network, what gives?"));
        }
    }
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

void setup()
{
  delay(1000);
  Serial.begin(115200);
  Serial.println();

  pinMode(TAIL_LED, OUTPUT);
  pinMode(BACK_FOOT_LED, OUTPUT);
  pinMode(FRONT_FOOT_LED, OUTPUT);
  pinMode(NOSE_LED, OUTPUT);
  pinMode(EYE_R, OUTPUT);
  pinMode(EYE_G, OUTPUT);
  pinMode(EYE_B, OUTPUT);

  setupWiFi();

  server.on("/", handleRoot);
  server.on("/flag", handleFlag);
  server.on("/blink", handleBlink);
  server.on("/rgb", handleRGB);

  server.on("/update", HTTP_POST, [](){
      server.sendHeader("Connection", "close");
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "text/plain", (Update.hasError())?"FAIL":"OK");
      ESP.restart();
    });

  server.onFileUpload(handleFileUpload);

  server.begin();

  Serial.println(F("HTTP server started"));
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
