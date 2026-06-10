#include <Arduino.h>
#include <Wire.h>
#include <LIS3MDL.h>
#include <SoftwareSerial.h>
#include <SD.h>
#include <RTClib.h>
#include <M2M_LM75A.h>

// inicjacja
LIS3MDL magnetometer;
File pliczek;
RTC_DS1307 rtc;
M2M_LM75A termometrIn(0x4B);
M2M_LM75A termometrOut(0x48);
uint32_t index = 0;
uint32_t independentIndex = 0;

// stany
bool mon = false;
bool sdon = false;
bool rtcon = false;

bool sdonPrev = false;

// zmienne potrzebne

int32_t mx = 0;
int32_t my = 0;
int32_t mz = 0;

class Module {
  public:
    void check() {
      if (!magnetometer.init()) {
        Serial.println(F("Brak łączności do magnetometru."));
        mon = false;
      }
      else {
        magnetometer.enableDefault();
        mon = true;
      }
      if (!SD.begin(10)) {
        Serial.println(F("Brak łączności do karty SD."));
        sdon = false;
      }
      else {
        sdon = true;
      }
      if (!rtc.begin()) {
        Serial.println("Nie znaleziono RTC.");
        rtcon = false;
      }
      else {
        rtcon = true;
        if (!rtc.isrunning()) {
          Serial.println(F("Moduł zegara RTC utracił obecnie zasilanie, ustawiam czas ostatniej kompilacji kodu na Arduino."));
          rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        }
      }
    }

    float freezingTemperatures(float temp) {
      if (temp > 150) {
        return temp - 256;
      }
      else {
        return temp;
      }
    }
    
    void printData() {
      if (!pliczek) return;
      if (pliczek.size() == 0) {
        pliczek.println(F("// Numer, godzina, magnetometr x, magnetometr y, magnetometr z, temperatura środka (interior), temperatura zewnątrz (outside), niezależny wskaźnik życia (+1 per 200 ms)"));
        pliczek.println(F("index,czas,mx,my,mz,tInt,tOut,indexIndp"));
      }
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
      pliczek.print(mx);
      pliczek.print(F(","));
      pliczek.print(my);
      pliczek.print(F(","));
      pliczek.print(mz);
      pliczek.print(F(","));
      pliczek.print(freezingTemperatures(termometrIn.getTemperature()));
      pliczek.print(F(","));
      pliczek.print(freezingTemperatures(termometrOut.getTemperature()));
      pliczek.print(F(","));
      pliczek.print(independentIndex);
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
  if (sdon && sdon != sdonPrev) {
    pliczek = SD.open("data.csv", FILE_WRITE);
    sdonPrev = true;
  }
  else if (!sdon) {
    sdonPrev = false;
  }
  termometrIn.begin();
  termometrOut.begin();
}

void loop() {
  independentIndex++;
  if (independentIndex % 15 == 0 && independentIndex != 0) {
    module.check();
  }
  if (mon) {
    magnetometer.read();
    module.printData();
  }
  if (sdon && sdon != sdonPrev) {
    pliczek = SD.open("data.csv", FILE_WRITE);
    sdonPrev = true;
  }
  else if (!sdon) {
    sdonPrev = false;
  }
  delay(200);
}