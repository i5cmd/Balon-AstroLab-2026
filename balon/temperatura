#include <Wire.h>

// adres czujnika TCN75A
int sensorAddress = 0x4B;

void setup()
{
  // uruchamia komunikację I2C
  Wire.begin();

  // uruchamia monitor portu szeregowego
  Serial.begin(9600);

  Serial.println("Start pomiaru temperatury");
}

void loop()
{
  // mówi czujnikowi:
  // "chcę odczytać temperaturę"
  Wire.beginTransmission(sensorAddress);

  // rejestr temperatury
  Wire.write(0x00);

  Wire.endTransmission();

  // pobiera 2 bajty danych z czujnika
  Wire.requestFrom(sensorAddress, 2);

  // sprawdza czy dane przyszły
  if (Wire.available() == 2)
  {
    int msb = Wire.read();
    int lsb = Wire.read();

    // zamiana danych na temperaturę
    float temperature = ((msb << 8) | lsb) / 256.0;

    
    Serial.print(temperature);
    Serial.println(" ");
  }

  delay(200);
}
