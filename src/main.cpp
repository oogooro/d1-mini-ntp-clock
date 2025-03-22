#include <Arduino.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <TM1650.h>
#include "secrets.h"
#include <Timezone.h>

TM1650 display;

WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, "pl.pool.ntp.org", 0, 60000);

TimeChangeRule dstRule = { "CEST", Last, Sun, Mar, 2, 120 };
TimeChangeRule stdRule = { "CET", Last, Sun, Oct, 3, 60 };

Timezone tz(dstRule, stdRule);

const char connecting[] = {
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
  display.init();
  display.setBrightness(1);

  WiFi.setHostname("ESP-CLOCK");
  WiFi.begin(SECRET_SSID, SECRET_PASSWORD);

  Serial.printf("Connecting to: %s", SECRET_SSID);

  while (WiFi.status() != WL_CONNECTED) {
    for (uint8_t i = 0; i <= 5; i++) {
      for (byte k = 0; k < 4; k++) {
        display.setPosition(k, connecting[i]);
      }
      delay(250);
      Serial.print(".");
    }
  }

  Serial.print('\n');
  Serial.println("Connected.");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  Serial.print('\n');

  timeClient.begin();
}

bool dotOn = true;
bool networkLostMessageShown = false;
u_long timestamp = millis();
uint16_t previousSensorValue = 0;
const uint8_t sensorFilteringThreshold = 10;

void loop() {
  uint16_t sensorValue = analogRead(A0);

  if (abs(sensorValue - previousSensorValue) > sensorFilteringThreshold) {
    uint8_t brightness = map(sensorValue, 0, 1023, 1, 7);
    display.setBrightness(brightness);
    previousSensorValue = sensorValue;
  }

  if (1000UL <= millis() - timestamp) {
    timestamp = millis();

    if (WiFi.status() == WL_CONNECTED) {
      bool timeUpdateSuccess = timeClient.update();
      if (timeUpdateSuccess) setTime(tz.toLocal(timeClient.getEpochTime()));
    }

    if (WiFi.status() != WL_CONNECTED) {
      if (!networkLostMessageShown) {
        networkLostMessageShown = true;
        display.displayString("NEt ");
        delay(2000);

        display.displayString("LOSt");
        delay(2000);
      }
    } else {
      networkLostMessageShown = false;
    }

    int hours = hour();
    int minutes = minute();

    String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);
    String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

    char buffer[5];

    (hoursStr + minuteStr).toCharArray(buffer, 5);

    display.displayString(buffer);

    display.setDot(1, dotOn);
    dotOn = !dotOn;
  }

  delay(50); // for stability
}
