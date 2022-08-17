#include <Wire.h>   //I2C library
#include "RTClib.h" //RTC library

RTC_DS1307 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

uint8_t wake_intervals[4] = {0, 10, 0, 0};

// Set wake up offset using Minute and Second
// To wake every 15 minutes start from 12:05, set wake_intervals[4] = {0, 0, 15, 0}, wake_offset = {0, 5}
// will wake 12:20, 12:35, 12:50, 1:05, 1:20
uint8_t wake_offset[2] = {0, 0};

// DS3231 alarm time
uint8_t wake_TIME[4] = {0, 0, 0, 0};

uint8_t previous_wake_TIME[4] = {0, 0, 0, 0};

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  Wire.begin();
  while (!Serial)
    ; // wait for serial
  delay(200);
  Serial.println("DS1307RTC Read Test");
  Serial.println("-------------------");

  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");
    while (1)
      ;
  }
  if (!rtc.isrunning())
  {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  DateTime now = rtc.now();
  previous_wake_TIME[0] = now.second();
  previous_wake_TIME[1] = now.minute();
  previous_wake_TIME[2] = now.hour();

  Serial.print(previous_wake_TIME[2], DEC);
  Serial.print(':');
  Serial.print(previous_wake_TIME[1], DEC);
  Serial.print(':');
  Serial.print(previous_wake_TIME[0], DEC);
  Serial.println();
}

void loop()
{
  // put your main code here, to run repeatedly:
  for (int i = 0; i < 4; i++)
  {
    wake_TIME[i] = previous_wake_TIME[i];
    wake_TIME[i] = wake_TIME[i] + wake_intervals[i];
  }

  for (int i = 0; i < 2; i++)
  {

    // calculate second and hour offset
    if (wake_intervals[i] != 0)
    {
      wake_TIME[i] = wake_TIME[i] - wake_TIME[i] % wake_intervals[i];
      wake_TIME[i] = wake_TIME[i] + wake_offset[i];
    }
    else
    {
      wake_TIME[i] = wake_offset[i];
    }

    // overflow increment for seconds and minutes
    if (wake_TIME[i] > 59)
    {
      wake_TIME[i + 1] = wake_TIME[i + 1] + 1;
      wake_TIME[i] = wake_TIME[i] - 60;
    }
  }

  // overflow increment for hours
  if (wake_TIME[2] > 23)
  {
    wake_TIME[2] = wake_TIME[2] - 24;
  }

  for (int i = 0; i < 4; i++)
  {
    previous_wake_TIME[i] = wake_TIME[i];
  }

  Serial.println("Next wake time");
  Serial.print(wake_TIME[2], DEC);
  Serial.print(':');
  Serial.print(wake_TIME[1], DEC);
  Serial.print(':');
  Serial.print(wake_TIME[0], DEC);
  Serial.println();

  delay(5000);
}
