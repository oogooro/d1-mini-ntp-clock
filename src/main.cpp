#include <Arduino.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <TM1650.h>
#include "secrets.h"

TM1650 d;

WiFiUDP ntpUDP;

const bool CZAS_ZIMOWY = true;

NTPClient timeClient(ntpUDP, "pl.pool.ntp.org", CZAS_ZIMOWY ? 3600 : 7200, 60000);

void setup() {
  pinMode(A0, INPUT);

  Serial.begin(115200);
  Serial.write("\n\n");

  Wire.begin();
  d.init();
  d.setBrightness(1);
  d.displayString("8888");
  d.setDot(1, true);

  WiFi.begin(SECRET_SSID, SECRET_PASSWORD);

  Serial.printf("Connecting to: %s", SECRET_SSID);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  Serial.print('\n');
  Serial.println("Connected.");
  Serial.print('\n');

  timeClient.begin();
}

bool dotOn = true;
bool networkLostMessageShown = false;

void loop() {
  if (WiFi.status() == WL_CONNECTED) timeClient.update();

  if (WiFi.status() != WL_CONNECTED) {
    if (!networkLostMessageShown) {
      networkLostMessageShown = true;
      d.displayString("NEt ");
      delay(2000);

      d.displayString("LOSt");
      delay(2000);
    }
  } else {
    networkLostMessageShown = false;
  }

  int sensor = analogRead(A0);
  int brightness = map(sensor, 1, 1024, 1, 7);

  d.setBrightness(brightness);

  int hours = timeClient.getHours();
  int minutes = timeClient.getMinutes();

  String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);
  String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

  char buffer[5];

  (hoursStr + minuteStr).toCharArray(buffer, 5);

  d.displayString(buffer);
  d.setDot(1, dotOn);
  dotOn = !dotOn;

  delay(500);
  d.setDot(1, dotOn);
  delay(500);
}
