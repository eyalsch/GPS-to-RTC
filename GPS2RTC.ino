#include <SPI.h>
#include <Wire.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define DS3231_I2C_ADDRESS 0x68
#define LEAP_YEAR(Y) ((Y>0) && !(Y%4) && ( (Y%100) || !(Y%400) )) // from time-lib
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

static const int RXPin = 4, TXPin = 3;
static const uint32_t GPSBaud = 9600;
TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);
byte second, minute, hour, dayOfWeek, day, month, year;

void setup () {
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();
  delay(100);

  readDS3231time(&second, &minute, &hour, &dayOfWeek, &day, &month,  &year);
  Serial.print("RTC Time: ");
  Serial.print(day < 10 ? "0" : "");
  Serial.print(day);
  Serial.print("/");
  Serial.print(month < 10 ? "0" : "");
  Serial.print(month);
  Serial.print("/");
  Serial.print(2000 + year);
  Serial.print("  ");
  Serial.print(hour < 10 ? "0" : "");
  Serial.print(hour);
  Serial.print(":");
  Serial.print(minute < 10 ? "0" : "");
  Serial.print(minute);
  Serial.print(":");
  Serial.print(second < 10 ? "0" : "");
  Serial.println(second);

  ss.begin(GPSBaud);
}

void loop () {

  while (ss.available() > 0)
    if (gps.encode(ss.read()))
    {
      if (gps.date.isValid() && gps.time.isValid())
      {
        dayOfWeek=CalculateDayOfWeek(gps.date.year() + 2000, gps.date.month(), gps.date.day());
        Serial.println(dayOfWeek);
        setDS3231time(gps.time.second(), gps.time.minute(), gps.time.hour() + 2, dayOfWeek-1, gps.date.day(), gps.date.month(), gps.date.year()-2000);
        
        Serial.print("GPS Time: ");
        if (gps.date.day() < 10) Serial.print(F("0"));
        Serial.print(gps.date.day());
        Serial.print(F("/"));
        if (gps.date.month() < 10) Serial.print(F("0"));
        Serial.print(gps.date.month());
        Serial.print(F("/"));
        Serial.print(gps.date.year());
        Serial.print(F("  "));
        if (gps.time.hour() < 10) Serial.print(F("0"));
        Serial.print(gps.time.hour());
        Serial.print(F(":"));
        if (gps.time.minute() < 10) Serial.print(F("0"));
        Serial.print(gps.time.minute());
        Serial.print(F(":"));
        if (gps.time.second() < 10) Serial.print(F("0"));
        Serial.println(gps.time.second());
        
        while (true);
      }

    }

  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS detected: check wiring."));
    while (true);
  }
}


byte decToBcd(byte val)
{
  return ((val / 10 * 16) + (val % 10));
}

byte bcdToDec(byte val)
{
  return ((val / 16 * 10) + (val % 16));
}

void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0);
  Wire.write(decToBcd(second));
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(hour));
  Wire.write(decToBcd(dayOfWeek));
  Wire.write(decToBcd(dayOfMonth));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));
  Wire.endTransmission();
}

void readDS3231time(byte *second, byte *minute, byte *hour, byte *dayOfWeek, byte *dayOfMonth, byte *month, byte *year)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0);
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);

  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}

byte CalculateDayOfWeek(byte year, byte month, byte day)
{
  byte months[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };   // days until 1st of month

  uint32_t days = year * 365;        // days until year
  for (int i = 4; i < year; i += 4) if (LEAP_YEAR(i) ) days++;     // adjust leap years, test only multiple of 4 of course

  days += months[month - 1] + day;  // add the days of this year
  if ((month > 2) && LEAP_YEAR(year)) days++;  // adjust 1 if this year is a leap year, but only after febr

  return days % 7;   // remove all multiples of 7
}
