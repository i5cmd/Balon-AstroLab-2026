#include <Arduino.h>
#include <Wire.h>
#include <LIS3MDL.h>

LIS3MDL magnetometer;

int works = false;

class Kompas {
  private:
    int xMin = 32676; // haha 67 no itd.
    int xMax = -32676;
    int yMin = 32676;
    int yMax = -32676;
    int zMin = 32676;
    int zMax = -32676;

    int xOffset = 0;
    int yOffset = 0;
    int zOffset = 0;

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

    void sprawdzMagnetometr() {
      if (!magnetometer.init()) {
        if (!works) {
          Serial.println(F("Nie znaleziono magnetometru."));
          while (1);
        }
      }
      else {
        magnetometer.enableDefault();
      }
    }
    
    void printData() {
      Serial.print(millis());
      Serial.print(F(","));
      Serial.print(magnetometer.m.x);
      Serial.print(F(","));
      Serial.print(magnetometer.m.y);
      Serial.print(F(","));
      Serial.println(magnetometer.m.z);
    }
};

Kompas kompas;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  delay(30);
  Wire.begin();
  Wire.setWireTimeout(250000, true);
  delay(30);
  kompas.sprawdzMagnetometr();
  Serial.println(F("Start pracy magnetometru."));
  works = true;
}

void loop() {
  magnetometer.read();
  kompas.printData();
  delay(200);
}
