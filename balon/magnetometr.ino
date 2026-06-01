#include <Arduino.h>
#include <Wire.h>
#include <LIS3MDL.h>
#include <SoftwareSerial.h>
#include <SD.h>

LIS3MDL magnetometer;
SoftwareSerial porty(2, 3);
File pliczek;

int works = false;

class Kompas {
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

    void sprawdzMagnetometrIPorty() {
      if (!magnetometer.init()) {
        if (!works) {
          Serial.println(F("Nie znaleziono magnetometru."));
          while (1);
        }
      }
      else {
        magnetometer.enableDefault();
      }
      if (!SD.begin(4)) {
        Serial.println(F("Nie znaleziono karty SD."));
        while (1);
      }
    }
    
    void printData() {
      if (!pliczek) return;
      pliczek.print(millis());
      pliczek.print(F(","));
      pliczek.print(magnetometer.m.x);
      pliczek.print(F(","));
      pliczek.print(magnetometer.m.y);
      pliczek.print(F(","));
      pliczek.println(magnetometer.m.z);
      pliczek.flush();
    }
};

Kompas kompas;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  porty.begin(9600);
  delay(30);
  Wire.begin();
  Wire.setWireTimeout(250000, true);
  delay(30);
  kompas.sprawdzMagnetometrIPorty();
  Serial.println(F("Start pracy magnetometru."));
  pliczek = SD.open("data.csv", FILE_WRITE);
  works = true;
}

void loop() {
  if (porty.available() > 0) {
    char impuls = porty.read();
    porty.write(impuls);
    magnetometer.read();
    kompas.printData();
  }
}
