#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <TimeLib.h>
#include <NtpClientLib.h>

#include <LedControl.h>  // http://wayoda.github.io/LedControl/pages/software

#include <ESP8266WiFi.h>          // Replace with WiFi.h for ESP32
#include <ESP8266WebServer.h>     // Replace with WebServer.h for ESP32
#include <AutoConnect.h>

ESP8266WebServer Server;          // Replace with WebServer for ESP32
AutoConnect      Portal(Server);

/* Create a new LedControl variable.
 * We use pins 12,11 and 10 on the Arduino for the SPI interface
 * Pin 12 is connected to the DATA IN-pin of the first MAX7221
 * Pin 11 is connected to the CLK-pin of the first MAX7221
 * Pin 10 is connected to the LOAD(/CS)-pin of the first MAX7221
 * There will only be a single MAX7221 attached to the arduino 
 */  
LedControl lc = LedControl(D7, D6, D5, 2);

void rootPage() {
  char content[] = "Hello, world";
  Server.send(200, "text/plain", content);
}



#define ONBOARDLED 2 // Built in LED on ESP-12/ESP-07
#define SHOW_TIME_PERIOD 5000
#define NTP_TIMEOUT 1500

int8_t timeZone = 1;
int8_t minutesTimeZone = 0;
const PROGMEM char *ntpServer = "pool.ntp.org";
bool wifiFirstConnected = false;

boolean syncEventTriggered = false; // True if a time even has been triggered
NTPSyncEvent_t ntpEvent; // Last triggered event

void onSTAConnected (WiFiEventStationModeConnected ipInfo) {
  Serial.printf ("Connected to %s\r\n", ipInfo.ssid.c_str ());
}


// Start NTP only after IP network is connected
void onSTAGotIP (WiFiEventStationModeGotIP ipInfo) {
  Serial.printf ("Got IP: %s\r\n", ipInfo.ip.toString ().c_str ());
  Serial.printf ("Connected: %s\r\n", WiFi.status () == WL_CONNECTED ? "yes" : "no");
  digitalWrite (ONBOARDLED, LOW); // Turn on LED
  wifiFirstConnected = true;
}

// Manage network disconnection
void onSTADisconnected (WiFiEventStationModeDisconnected event_info) {
  Serial.printf ("Disconnected from SSID: %s\n", event_info.ssid.c_str ());
  Serial.printf ("Reason: %d\n", event_info.reason);
  digitalWrite (ONBOARDLED, HIGH); // Turn off LED
  //NTP.stop(); // NTP sync can be disabled to avoid sync errors
  WiFi.reconnect ();
}

void processSyncEvent (NTPSyncEvent_t ntpEvent) {
  if (ntpEvent < 0) {
    Serial.printf ("Time Sync error: %d\n", ntpEvent);
    if (ntpEvent == noResponse)
      Serial.println ("NTP server not reachable");
    else if (ntpEvent == invalidAddress)
      Serial.println ("Invalid NTP server address");
    //else if (ntpEvent == errorSending)
    //    Serial.println ("Error sending request");
    //else if (ntpEvent == responseError)
    //    Serial.println ("NTP response error");
  } else {
    if (ntpEvent == timeSyncd) {
      Serial.print ("Got NTP time: ");
      Serial.println (NTP.getTimeDateString (NTP.getLastNTPSync ()));
    }
  }
}

void setup() {

  static WiFiEventHandler e1, e2, e3;

  NTP.onNTPSyncEvent ([](NTPSyncEvent_t event) {
    ntpEvent = event;
    syncEventTriggered = true;
  });

  e1 = WiFi.onStationModeGotIP (onSTAGotIP);// As soon WiFi is connected, start NTP Client
  e2 = WiFi.onStationModeDisconnected (onSTADisconnected);
  e3 = WiFi.onStationModeConnected (onSTAConnected);

  Serial.begin(115200);
  Serial.println("Booting");
  /*
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
    }
  */

  Server.on("/", rootPage);
  if (Portal.begin()) {
    Serial.println("WiFi connected: " + WiFi.localIP().toString());
  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("NodeMCU_8266_1");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");


  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  lc.shutdown(0, false);
  lc.shutdown(1, false);

  //set a medium brightness for the Leds
  lc.setIntensity(0, 8);
  lc.setIntensity(1, 2);


}


void setHour(uint8_t hour, uint8_t inner, bool value)
{
  hour = (hour + 11) % 12;


  /*if (hour != 0)
    {
    if (hour > 12) hour = hour - 12;
    hour = 12 - hour;
    }*/

  //                          00 01 02 03 04 05 06 07  08 09 10 11  12 13 14 15 16 17 18 19 20 21 22 23
  uint8_t slotMapping1[24] = { 0, 0, 0, 0, 0, 0, 0, 0,  4, 4, 4, 4,  1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 4 };
  uint8_t ledMapping1[24] =  { 4, 0, 5, 3, 7, 2, 6, 1,  4, 0, 5, 3,  7, 2, 6, 1, 4, 0, 5, 3, 7, 2, 6, 1 };

  uint8_t slotMapping2[24] = { 4, 4, 4, 4, 6, 6, 6, 6,  6, 6, 6, 6,  1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 4 };
  uint8_t ledMapping2[24] =  { 7, 2, 6, 1, 4, 0, 5, 3,  7, 2, 6, 1,  4, 0, 5, 3, 7, 2, 6, 1, 4, 0, 5, 3 };


  uint8_t slot = 0;
  uint8_t led = 0;

  if (inner == 0)
  {
    slot = slotMapping1[hour];
    led = ledMapping1[hour];
  }
  else
  {
    slot = slotMapping2[hour];
    led = ledMapping2[hour];
  }

  lc.setLed(1, slot, led, value);



}

int setMinute(int min, bool value)
{
  min = 60 - min;

  if (min == 0) min = 59;
  else min = min - 1;

  int slot = min / 8 + 1;
  int led = min % 8 + 1;

  switch (led)
  {
    // LED
    case 5: led = 7; break;
    case 7: led = 6; break;
    case 3: led = 5; break;
    case 1: led = 4; break;
    case 4: led = 3; break;
    case 6: led = 2; break;
    case 8: led = 1; break;
    case 2: led = 0; break;
  }



  switch (slot)
  {
    case 5: slot = 0; break;
    case 1: slot = 1; break;
    case 8: slot = 2; break;
    case 4: slot = 3; break;
    case 6: slot = 4; break;
    case 2: slot = 5; break;
    case 7: slot = 6; break;
    case 3: slot = 7; break;
  }


  lc.setLed(0, slot, led, value);
}



int lastMin = -1;
int lastSec = -1;
int lastHour = -1;


void loop() {
  ArduinoOTA.handle();
  Portal.handleClient();

  if (wifiFirstConnected) {
    wifiFirstConnected = false;
    NTP.setInterval (63);
    NTP.begin (ntpServer, timeZone, true, minutesTimeZone);
  }

  if (syncEventTriggered) {
    processSyncEvent (ntpEvent);
    syncEventTriggered = false;
  }


  int currSec = second();
  int currentMin = minute();
  int currHour = hour();
  
  if (currSec != lastSec ) 
  {
    Serial.println (NTP.getTimeDateString ());

    //
    // Handle hours
    //
    if ( currHour != lastHour )
    {
      if ( lastHour != -1 )
      {
        setHour ( lastHour, 0, false );
        setHour ( lastHour, 1, false );
      }
      lastHour = currHour;

      setHour ( currHour, 0, true );
      setHour ( currHour, 1, true );
    }

    //
    // Handle minutes
    //
    if ( currentMin != lastMin )
    {
      if ( lastMin != -1 )
      {
        setMinute ( lastMin, false );
      }
      lastMin = currentMin;

      setMinute ( currentMin, true );
    }


    //
    // Handle seconds
    //    
    if ( currSec != lastSec )
    {
      if (( currSec != -1 )  && (lastSec != currentMin))
      {
        setMinute ( lastSec, false );
      }
      lastSec = currSec;

      setMinute ( currSec, true );
    }    
  }
}
