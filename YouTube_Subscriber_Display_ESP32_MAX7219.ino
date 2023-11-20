#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <WiFi.h>

#include <HardwareSerial.h>

#include "src/youtube_stats.h"
#include "src/wifi_credentials.h"
#include "Arduino.h"

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 8 // 8 blocks
#define CS_PIN 21

#define ZONE_RIGHT 0
#define ZONE_LEFT 1

#define DELAY 15000

#define INTERRUPT_PIN_1 26
#define INTERRUPT_PIN_2 27
#define NUM_BRIGHTNESS_LEVELS 3

// globals
MD_Parola ledMatrix = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
YouTubeStats ytStats;

int brightnessLevels[NUM_BRIGHTNESS_LEVELS] = {0, 3, 8};
int brightnessIndex = 0;

enum DisplayMode {
  SUBSCRIBERS,
  VIEWS,
  VIDEOS
};

volatile DisplayMode displayMode = SUBSCRIBERS;

void IRAM_ATTR buttonOneISR() { // brightness
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 200) {
    brightnessIndex = (brightnessIndex + 1) % NUM_BRIGHTNESS_LEVELS;
    int brightness = brightnessLevels[brightnessIndex];

    ledMatrix.setIntensity(ZONE_LEFT, brightness);
    ledMatrix.setIntensity(ZONE_RIGHT, brightness);
    Serial.println("Set brightness to " + String(brightness)); 
  }
  last_interrupt_time = interrupt_time;
}

void IRAM_ATTR buttonTwoISR() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 200) {
    if (displayMode < VIDEOS) {
      displayMode = static_cast<DisplayMode>(displayMode + 1);
    } else {
      displayMode = SUBSCRIBERS;
    }
    Serial.println("Switched DisplayMode");
  }
  displayStats();
  last_interrupt_time = interrupt_time;
}

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
        ledMatrix.displayClear(ZONE_LEFT);
        ledMatrix.displayClear(ZONE_RIGHT);
      }
    }
  }
}


void setup() {
  Serial.begin(115200);
  ledMatrix.begin(2);         // initialize the object
  ledMatrix.displayClear();  // clear led matrix display
  ledMatrix.setTextAlignment(PA_CENTER);
  ledMatrix.setZone(0, 0, MAX_DEVICES - 1); // set to single zone for now
  int brightness = brightnessLevels[brightnessIndex];
  ledMatrix.setIntensity(ZONE_LEFT, brightness);
  ledMatrix.setIntensity(ZONE_RIGHT, brightness);
  pinMode(INTERRUPT_PIN_1, INPUT_PULLDOWN);
  pinMode(INTERRUPT_PIN_2, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN_1), buttonOneISR, RISING);
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN_2), buttonTwoISR, RISING);
}

void loop() {
  ensureWiFiConnection();

  // fetch stats
  static unsigned long last_query_time = 0;
  unsigned long current_time = millis();
  if (current_time - last_query_time > DELAY) { // if delay has passed, do the query
      if (ytStats.fetch()) {
        Serial.println("YouTube stats fetched");
        last_query_time = current_time;
      } else {
        Serial.println("Failed to fetch YouTube stats");
        Serial.println(ytStats.getError());
        ledMatrix.setZone(0, 0, MAX_DEVICES - 1);
        ledMatrix.displayScroll(ytStats.getError().c_str(), PA_CENTER, PA_SCROLL_LEFT, 100);
        unsigned long startTime = millis();
        while(ledMatrix.displayAnimate() && millis() - startTime < DELAY * 2); // Timeout after delay
        ledMatrix.displayReset();
        ledMatrix.displayClear(ZONE_LEFT);
        ledMatrix.displayClear(ZONE_RIGHT);
      }
  }
  displayStats();
}
void displayStats() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  // display stats
  ledMatrix.setZone(ZONE_RIGHT, 0, (MAX_DEVICES / 2 - 1) + 1);          // Right half of the display + 1
  ledMatrix.setZone(ZONE_LEFT, (MAX_DEVICES / 2) + 1, MAX_DEVICES - 1); // Left half of the display 

  ledMatrix.displayReset(ZONE_LEFT);
  ledMatrix.displayReset(ZONE_RIGHT);

  char countStr[10];
  switch (displayMode)
  {
  case SUBSCRIBERS:
    sprintf(countStr, "%d", ytStats.getEstSubscriberCount());
    ledMatrix.displayZoneText(ZONE_LEFT, "Subs:", PA_LEFT, 100, 0, PA_PRINT, PA_NO_EFFECT);
    break;
  case VIEWS:
    sprintf(countStr, "%d", ytStats.getViewCount());
    ledMatrix.displayZoneText(ZONE_LEFT, "Vws:", PA_LEFT, 100, 0, PA_PRINT, PA_NO_EFFECT);
    break;
  case VIDEOS:
    sprintf(countStr, "%d", ytStats.getVideoCount());
    ledMatrix.displayZoneText(ZONE_LEFT, "Vids:", PA_LEFT, 100, 0, PA_PRINT, PA_NO_EFFECT);
    break;
  }

  ledMatrix.displayZoneText(ZONE_RIGHT, countStr, PA_RIGHT, 100, 0, PA_PRINT, PA_NO_EFFECT);

  ledMatrix.synchZoneStart();
  // wait until all done
  bool allDone = false;
  while (!allDone)
  {
    if (ledMatrix.displayAnimate())
    {
      allDone = ledMatrix.getZoneStatus(ZONE_RIGHT) && ledMatrix.getZoneStatus(ZONE_LEFT);
    }
  }
}
