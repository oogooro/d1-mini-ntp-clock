#include <Arduino.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <TM1650.h>
#include "secrets.h"
#include <EEPROM.h>

TM1650 d;

WiFiUDP ntpUDP;

int timeOffset = 3600;

NTPClient timeClient(ntpUDP, "pl.pool.ntp.org", timeOffset, 60000);

const byte loading[] = {
    0b00000001,
    0b00000010,
    0b00000100,
    0b00001000,
    0b00010000,
    0b00100000,
};

void setup() {
  pinMode(A0, INPUT);

  Serial.begin(115200);
  Serial.write("\n\n");

  Wire.begin();
  d.init();
  d.setBrightness(1);

  WiFi.begin(SECRET_SSID, SECRET_PASSWORD);

  Serial.printf("Connecting to: %s", SECRET_SSID);

  byte step = 0; 
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");

    for (byte i = 0; i < 4; i++)
      d.setPosition(i, loading[step]);

    step++;
    if (step >= 6) step = 0;
  }

  Serial.print('\n');
  Serial.println("Connected.");
  Serial.print('\n');

  timeClient.begin();

  Serial.println("Timezone -1: , | Timezone +1: . | Timezone 0: / | Write changes: [RETURN]");

  EEPROM.begin(4);
  EEPROM.get(0, timeOffset);

  if (timeOffset == -1) {
    timeOffset = 0;
    EEPROM.put(0, timeOffset);
    EEPROM.commit();
  }

  timeClient.setTimeOffset(timeOffset);
}

bool dotOn = true;
bool networkLostMessageShown = false;
ulong timestamp = millis();

void loop() {
  int sensor = analogRead(A0);
  int brightness = map(sensor, 1, 1024, 1, 7);

  d.setBrightness(brightness);

  if (1000UL <= millis() - timestamp) {
    timestamp = millis();

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

    int hours = timeClient.getHours();
    int minutes = timeClient.getMinutes();

    String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);
    String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

    char buffer[5];

    (hoursStr + minuteStr).toCharArray(buffer, 5);

    d.displayString(buffer);
    
    d.setDot(1, dotOn);
    dotOn = !dotOn;
  }

  while (Serial.available() > 0) {
    char command = Serial.read();

    if (command == ',') {
      timeOffset -= 3600;
      timeClient.setTimeOffset(timeOffset);
      Serial.print("Timezone offset: ");
      Serial.println(timeOffset);
    } else if (command == '.') {
      timeOffset += 3600;
      timeClient.setTimeOffset(timeOffset);
      Serial.print("Timezone offset: ");
      Serial.println(timeOffset);
    } else if (command == '/') {
      timeOffset = 0;
      timeClient.setTimeOffset(timeOffset);
      Serial.print("Timezone offset: ");
      Serial.println(timeOffset);
    } else if (command == '\n') {
      EEPROM.put(0, timeOffset);

      if (EEPROM.commit())
        Serial.println("Wrote to EEPROM");
      else
        Serial.println("Failed to write to EEPROM");
    }
  }

  delay(50); // for stability
}
