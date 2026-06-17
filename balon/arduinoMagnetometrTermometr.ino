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
File czas;
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

class LastTime {
  public:
    int32_t year = 2026;
    int32_t month = 6;
    int32_t day = 17;
    int32_t hour = 10;
    int32_t minute = 0;
    int32_t second = 0;
};

LastTime lastTime;

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
          //rtc.adjust(DateTime(2026, 6, 17, 10, 0, 0));
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

      czas = SD.open("time.txt", O_WRITE | O_CREAT | O_TRUNC);
      czas.print(date.year(), DEC);
      czas.print(F(","));
      czas.print(date.month(), DEC);
      czas.print(F(","));
      czas.print(date.day(), DEC);
      czas.print(F(","));
      czas.print(date.hour(), DEC);
      czas.print(F(","));
      czas.print(date.minute(), DEC);
      czas.print(F(","));
      czas.print(date.second(), DEC);
      czas.close();
    }
    void setTime() {
      if (!czas || czas.size() == 0) {
        rtc.adjust(DateTime(2026, 6, 17, 10, 0, 0));
        return;
      }
      else if (czas.available()) {
        char linia[32];
        size_t n = czas.readBytesUntil('\n', linia, sizeof(linia) - 1);
        linia[n] = '\0';

        czas.close();

        lastTime.year = atoi(strtok(linia, ","));
        lastTime.month = atoi(strtok(NULL, ","));
        lastTime.day = atoi(strtok(NULL, ","));
        lastTime.hour = atoi(strtok(NULL, ","));
        lastTime.minute = atoi(strtok(NULL, ","));
        lastTime.second = atoi(strtok(NULL, ","));

        rtc.adjust(DateTime(lastTime.year, lastTime.month, lastTime.day, lastTime.hour, lastTime.minute, lastTime.second));
        return;
      }
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
    czas = SD.open("time.txt", FILE_READ);
    module.setTime();
    sdonPrev = true;
  }
  else if (!sdon) {
    sdonPrev = false;
  }
  termometrIn.begin();
  termometrOut.begin();
  // rtc.adjust(DateTime(2026, 6, 17, 10, 0, 0));
}

void loop() {
  independentIndex++;
  if (independentIndex % 15 == 0 && independentIndex != 0) {
    module.check();
  }
  if (mon) {
    magnetometer.read();
    mx = magnetometer.m.x;
    my = magnetometer.m.y;
    mz = magnetometer.m.z;
  }
  else {
    mx = 4040404;
    my = 4040404;
    mz = 4040404;
  }
  if (sdon) {
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
