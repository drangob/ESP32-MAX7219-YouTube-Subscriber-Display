#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <WiFi.h>

#include "src/youtube_stats.h"
#include "src/wifi_credentials.h"


#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 8 // 8 blocks
#define CS_PIN 21

#define ZONE_RIGHT 0
#define ZONE_LEFT 1

#define THIRTY_SECONDS 30000

// globals
MD_Parola ledMatrix = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

void ensureWiFiConnection(){
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  ledMatrix.displayReset();

  while (WiFi.status() != WL_CONNECTED) { // loop forever until we find WiFi
    for (int i = 0; i < numNetworks; i++) {
      ledMatrix.print("WiFi Scanning");
      bool networkFound = WiFi.scanNetworks(false, false, false, 300U, 0U, networks[i].ssid) > 0;

      if (networkFound) { // connect to network if found
        WiFi.begin(networks[i].ssid, networks[i].password);
        while (WiFi.status() != WL_CONNECTED) { // wait for connection to finish
          ledMatrix.print("WiFi Connecting");
          delay(500);
        }
        ledMatrix.print("WiFi Online!");
        ledMatrix.displayReset();
      }
    }
  }
}


void setup() {
  ledMatrix.begin(2);         // initialize the object
  ledMatrix.displayClear();  // clear led matrix display
  ledMatrix.setTextAlignment(PA_CENTER);
  ledMatrix.setZone(0, 0, MAX_DEVICES - 1); // set to single zone for now
  ledMatrix.setIntensity(0, 0);
}

void loop() {
  ensureWiFiConnection();

  YouTubeStats ytStats;
  if (ytStats.fetch()) {
    ledMatrix.setZone(ZONE_RIGHT, 0, MAX_DEVICES/2 - 1); // Right half of the display
    ledMatrix.setZone(ZONE_LEFT, MAX_DEVICES/2, MAX_DEVICES - 1); // Left half of the display
    ledMatrix.setIntensity(ZONE_LEFT, 0);
    ledMatrix.setIntensity(ZONE_RIGHT, 0);

    ledMatrix.displayReset(ZONE_LEFT);
    ledMatrix.displayReset(ZONE_RIGHT);
    ledMatrix.displayClear(ZONE_LEFT);
    ledMatrix.displayClear(ZONE_RIGHT);

    char sub_str[10];
    sprintf(sub_str, "%d", ytStats.getEstSubscriberCount());

    ledMatrix.displayZoneText(ZONE_RIGHT, sub_str, PA_RIGHT, 100, 0, PA_PRINT , PA_NO_EFFECT);
    ledMatrix.displayZoneText(ZONE_LEFT, "Subs:", PA_LEFT, 100, 0, PA_PRINT , PA_NO_EFFECT);

    ledMatrix.synchZoneStart();
    // wait until all done
    bool allDone = false;
    while (!allDone) {
      if (ledMatrix.displayAnimate()) {
        allDone == ledMatrix.getZoneStatus(ZONE_RIGHT) && ledMatrix.getZoneStatus(ZONE_LEFT);
      }
    }
    
    delay(THIRTY_SECONDS);
  } else {
    ledMatrix.setZone(0, 0, MAX_DEVICES - 1);
    ledMatrix.displayScroll(ytStats.getError().c_str(), PA_CENTER, PA_SCROLL_LEFT, 100);
    while(ledMatrix.displayAnimate()); // loop to complete animation
    ledMatrix.displayReset();
  }
}
