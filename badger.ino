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

/* Set these to your desired credentials. */
char ssid[60] = "BadgerNet";
ESP8266WebServer server(80);

String flag="BADGERMASTER";
String myhostname="none";

const char* serverIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";

void handleRoot()
{
	server.send(200, "text/html", "<h1>You are connected</h1>");
}

void handleFlag()
{
  if(server.hasArg("newflag"))
  {
    flag=server.arg("newflag");
  }
  
  server.send(200, "text/html",flag);
}

void handleBlink()
{
  String device=server.arg("d");
  String state=server.arg("s");

  server.send(200, "text/html", "setting LED s= "+state+"  d="+device);
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
    Serial.println("Looking for more badgers.."); 
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; ++i)
    {
      char this_ssid[60];
      String string_ssid=WiFi.SSID(i);
      string_ssid.toCharArray(this_ssid,60);
      if(string_ssid.startsWith("BadgerNet"))
      {
        Serial.print("Trying to connect to "); 
        Serial.println(this_ssid); 
        WiFi.begin(this_ssid);
        if(WiFi.waitForConnectResult()==WL_CONNECTED)
        {
          online=1;
          Serial.print("Connected to ");
          Serial.println(this_ssid);
          
          Serial.print("IP address: ");
          Serial.println(WiFi.localIP());
          break; // break out of the for loop
        }
        else
        {
          Serial.print("Could not connect to ");
          Serial.println(this_ssid);
        }
      }
      else
      {
        Serial.print("I don't care about ");
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
      Serial.print("Fine, I'll make my own AP at ");
      Serial.println(ssid);
      Serial.print("AP IP address: ");
      Serial.println(WiFi.softAPIP());
    }
    else
    {
      Serial.println("Couldn't even set up my own network, what gives?");
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

  setupWiFi();
  
	server.on("/", handleRoot);
  server.on("/flag", handleFlag);
  server.on("/blink", handleBlink);
  
  server.on("/update", HTTP_POST, [](){
    server.sendHeader("Connection", "close");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", (Update.hasError())?"FAIL":"OK");
    ESP.restart();
  });
  
  server.onFileUpload(handleFileUpload);
  
	server.begin();

	Serial.println("HTTP server started");
}

void loop() {
	server.handleClient();
}
