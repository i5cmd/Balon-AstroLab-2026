#include <Arduino.h>
#include <Wire.h>
#include <LIS3MDL.h>
#include <TimeLib.h>

unsigned long liczba = 0;
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
    String kierunek = "N";

    int xCenter = 0;
    int yCenter = 0;
    int zCenter = 0;
    
    int deg = 0;
    int oldDeg = 0;
    int lDeg = 0;

    void zamienStopnieNegatywne() {
      if (lDeg < 0) {
        lDeg += 360; // zamiana ujemnych stopnii na stopnie powyzej 180 stopni
      } 
    }

    int lerpDeg(int value, int oldValue) {
      int diff = value - oldValue;
      
      if (diff > 180) {
        diff -= 360;
      }
      else if (diff < -180) {
        diff += 360;
      }

      int lerpd = oldValue + 0.5 * (diff);
      
      return lerpd; 
    }

    void kalibrujKompas() {
      if (xMin > magnetometer.m.x) {
        xMin = magnetometer.m.x;
      }
      if (xMax < magnetometer.m.x) {
        xMax = magnetometer.m.x;
      }
      if (yMin > magnetometer.m.y) {
        yMin = magnetometer.m.y;
      }
      if (yMax < magnetometer.m.y) {
        yMax = magnetometer.m.y;
      }
      if (zMin > magnetometer.m.z) {
        zMin = magnetometer.m.z;
      }
      if (zMax < magnetometer.m.z) {
        zMax = magnetometer.m.z;
      }

      xOffset = (xMin + xMax) / 2;
      yOffset = (yMin + yMax) / 2;
      zOffset = (zMin + zMax) / 2;

      xCenter = magnetometer.m.x - xOffset;
      yCenter = magnetometer.m.y - yOffset;
      zCenter = magnetometer.m.z - zOffset;
    } 

    void ustawKierunkiSwiata() {
      if (lDeg < 22.5 || lDeg > 337.5) {
        kierunek = "N";
      }
      else if (lDeg <= 337.5 && lDeg >= 292.5) {
        kierunek = "NW";
      }
      else if (lDeg < 292.5 && lDeg > 247.5) {
        kierunek = "W";
      }
      else if (lDeg <= 247.5 && lDeg >= 202.5) {
        kierunek = "SW";
      }
      else if (lDeg < 202.5 && lDeg > 157.5) {
        kierunek = "S";
      }
      else if (lDeg < 157.5 && lDeg > 112.5) {
        kierunek = "SE";
      }
      else if (lDeg <= 112.5 && lDeg >= 67.5) {
        kierunek = "E";
      }
      else if (lDeg < 67.5 && lDeg > 22.5) {
        kierunek = "NE";
      }
    }

    void czasStart() {
      Serial.println("Przez pierwsze minuty kompas może nie być dokładny. Powinien jednak sam się ustabilizować po czasie.");
      delay(1000);
      Serial.println("5 sekund na ustawienie płaskiego ułożenia płytki.");
      delay(1000);
      Serial.println("5.....");
      delay(1000);
      Serial.println("4....");
      delay(1000);
      Serial.println("3...");
      delay(1000);
      Serial.println("2..");
      delay(1000);
      Serial.println("1.");
      delay(1000);
      Serial.println("# Start #");
    }
};

Kompas kompas;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  delay(30);
  Serial.println("---- Kompas Balona Stratosferycznego AstroLab 2026 ----");
  Wire.begin();
  Wire.setWireTimeout(250000, true);
  delay(30);
  sprawdzMagnetometr();
  delay(30);
  kompas.czasStart();
  works = true;
}

void loop() {
  if (magnetometer.init()) {
    magnetometer.read();
    kompas.kalibrujKompas();
    //kompas.deg = atan2(-kompas.yCenter, kompas.xCenter) * 180 / PI; // ZDOBYCIE STOPNI, y na minusie bo czytalem, znowu, ze y idzie w przeciwnym kierunku od wskazowek zegara. fuj. nieladnie. rzekomo to jedna z podstaw na matematyce, ale hej, ja mam na razie funkcje liniowe, give me a break...
    //kompas.lDeg = kompas.lerpDeg(kompas.deg, kompas.oldDeg);
    //kompas.zamienStopnieNegatywne();
    //kompas.ustawKierunkiSwiata();
    printData();
    //kompas.oldDeg = kompas.deg;
  }
  else {
    Serial.println("[!] Magnetometr został odłączony od płytki Arduino w trakcie wykonywania kodu.");
  }
  delay(200);
}

void sprawdzMagnetometr() {
  if (!magnetometer.init()) {
    if (!works) {
      Serial.println("[!] Nie znaleziono magnetometru.");
      while (1);
    }
  }
  else {
    magnetometer.enableDefault();
  }
}

void printData() {
  Serial.print(F("[.] {"));
  Serial.print(now());
  Serial.print("s} Współrzędne: X - ");
  Serial.print(kompas.xCenter);
  Serial.print(" / Y - ");
  Serial.print(kompas.yCenter);
  Serial.print(" / Z - ");
  Serial.print(kompas.zCenter);
  Serial.println(" / ");
  //Serial.print("s} Kierunek: ");
  //Serial.print(kompas.lDeg);
  //Serial.print("° - ");
  //Serial.print(kompas.kierunek);
  //Serial.print(" | Przechylenie: ");
  //Serial.println(kompas.zCenter);

  //----
  // Obrzydliwie to wygląda, przyznam, chciałbym napisać:
  // Serial.println(String("[.] {") + now() + String("s} Kierunek: ") + deg + String() + kierunek + String(" | Przechylenie: ") + );
  // Lecz z tego, co czytałem, to String() oraz sama konkatenacja to czarne konie.
  // Jak się dowiedziałem: w zwykłym komputerze nie ma to znaczenia, bo ma gigabajty RAMu. Ale w Arduino, która ma bardzo mało ram, może to powodować fragmentację RAMu spowodowaną tym, że płytka co ten delay szuka String() i sposobu na konkatenację. 
  // Jak mamy to wysyłać na kilka godzin 40 km nad ziemią, to lepiej, aby nic się po drodze nie zepsuło.
  // Dlatego rozdzieliłem to w taki sposób. I mean Arduino Nano, na którym to piszę, to mrówka. Dużo nóg ma, mała jest sama i mądra, ale przez rozmiar ma duże wady.
}