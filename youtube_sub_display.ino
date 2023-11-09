#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <WiFi.h>

#include "youtube_stats.h"
#include "wifi_credentials.h"


#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4 // 4 blocks
#define CS_PIN 21

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
      ledMatrix.print("Scan...");
      bool networkFound = WiFi.scanNetworks(false, false, false, 300U, 0U, networks[i].ssid) > 0;

      if (networkFound) { // connect to network if found
        WiFi.begin(networks[i].ssid, networks[i].password);
        while (WiFi.status() != WL_CONNECTED) { // wait for connection to finish
          ledMatrix.print("WiFi...");
          delay(500);
        }
        ledMatrix.print("Online!");
        ledMatrix.displayReset();
      }
    }
  }
}


void setup() {
  ledMatrix.begin();         // initialize the object
  ledMatrix.setIntensity(7); // set the brightness of the LED matrix display (from 0 to 15)
  ledMatrix.displayClear();  // clear led matrix display
  ledMatrix.setTextAlignment(PA_RIGHT);
}

void loop() {
  ensureWiFiConnection();

  YouTubeStats ytStats;
  if (ytStats.fetch()) {
    ledMatrix.print(ytStats.getSubscriberCount());
    delay(THIRTY_SECONDS);
  } else {
    ledMatrix.displayScroll(ytStats.getError().c_str(), PA_CENTER, PA_SCROLL_LEFT, 100);
    while(ledMatrix.displayAnimate()); // loop to complete animation
    ledMatrix.displayReset();
  }
}
