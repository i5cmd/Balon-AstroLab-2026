#include <Arduino.h>
#include <Wire.h>
#include <LIS3MDL.h>
#include <SoftwareSerial.h>
#include <SD.h>
#include <RTClib.h>
#include <M2M_LM75A.h>

LIS3MDL magnetometer;
File pliczek;
RTC_DS1307 rtc;
M2M_LM75A termometrIn(0x4B);
M2M_LM75A termometrOut(0x48);
int index = 0;

class Module {
  public:
    int xCenter = 0;
    int yCenter = 0;
    int zCenter = 0;

    /*void kalibrujKompas() {
      if (xMin > magnetometer.m.x) xMin = magnetometer.m.x;
      if (xMax < magnetometer.m.x) xMax = magnetometer.m.x;
      if (yMin > magnetometer.m.y) yMin = magnetometer.m.y;
      if (yMax < magnetometer.m.y) yMax = magnetometer.m.y;
      if (zMin > magnetometer.m.z) zMin = magnetometer.m.z;
      if (zMax < magnetometer.m.z) zMax = magnetometer.m.z;

      xOffset = (xMin + xMax) / 2;
      yOffset = (yMin + yMax) / 2;
      zOffset = (zMin + zMax) / 2;

      xCenter = magnetometer.m.x - xOffset;
      yCenter = magnetometer.m.y - yOffset;
      zCenter = magnetometer.m.z - zOffset;
    } */

    void check() {
      if (!magnetometer.init()) {
        Serial.println(F("Nie znaleziono magnetometru."));
        while (1);
      }
      else {
        magnetometer.enableDefault();
      }
      if (!SD.begin(10)) {
        Serial.println(F("Nie znaleziono karty SD."));
        while (1);
      }
      if (!rtc.begin()) {
        Serial.println("Nie znaleziono RTC.");
        while (1);
      }
      if (!rtc.isrunning()) {
        Serial.println(F("RTC utracił zasilanie, ustawiam czas kompilacji!"));
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      }
    }
    
    void printData() {
      if (!pliczek) return;
      DateTime date = rtc.now();
      index++;
      pliczek.print(index);
      pliczek.print(F(","));
      pliczek.print(date.hour(), DEC);
      pliczek.print(F(":"));
      pliczek.print(date.minute(), DEC);
      pliczek.print(F(":"));
      pliczek.print(date.second(), DEC);
      pliczek.print(F(","));
      pliczek.print(magnetometer.m.x);
      pliczek.print(F(","));
      pliczek.print(magnetometer.m.y);
      pliczek.print(F(","));
      pliczek.print(magnetometer.m.z);
      pliczek.print(F(","));
      pliczek.print(termometrIn.getTemperature());
      pliczek.print(F(","));
      pliczek.print(termometrOut.getTemperature());
      pliczek.println(F(""));
      pliczek.flush();
    }
};

Module module;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  delay(30);
  Wire.begin();
  Wire.setWireTimeout(250000, true);
  delay(30);
  module.check();
  Serial.println(F("Start pracy magnetometru, termometrów."));
  pliczek = SD.open("data.csv", FILE_WRITE);
  termometrIn.begin();
  termometrOut.begin();
}

void loop() {
    magnetometer.read();
    module.printData();
    delay(200);
}