#include <Wire.h>
#include <Alarm.h>
#include <ds3231.h>

struct ts t;

// DS3231 alarm time
uint8_t wake_TIME[4] = {0, 0, 0, 0};

uint8_t current_TIME[4] = {0, 0, 0, 0};

#define wakePin 2 // when low, makes 328P wake up, must be an interrupt pin (2 or 3 on ATMEGA328P)

void getCurrentTime(){
  DS3231_get(&t);
  current_TIME[0] = t.sec;
  current_TIME[1] = t.min;
  current_TIME[2] = t.hour;
}

boolean check_time(uint8_t wake_intervals[4], uint8_t wake_offset[2], uint8_t wake_tolerance)
{
  // put your main code here, to run repeatedly:
  // since tolerance means +-, so this is calculate the time distance from the previous and next interval

  long tolerance_seconds = wake_tolerance * 60;
  long previous_interval = 0;
  long next_interval = 0;

  for (int i = 2; i >= 0; i--)
  {
    int time_modulus = 0;
    int time_diff = 0;

    time_modulus = current_TIME[i] % wake_intervals[i];
    time_diff = wake_intervals[i] - time_modulus;

    previous_interval = previous_interval * 60 + time_modulus;
    next_interval = next_interval * 60 + time_diff;
  }

  // if both previous and next interval distance is more than tolerance seconds means do not deploy
  if (previous_interval > tolerance_seconds && next_interval > tolerance_seconds)
  {
    return false;
  }
  else
  {
    return true;
  }
}

void setNextAlarm(uint8_t wake_intervals[4], uint8_t wake_offset[2])
{
  // flags define what calendar component to be checked against the current time in order
  // to trigger the alarm - see datasheet
  // A1M1 (seconds) (0 to enable, 1 to disable)
  // A1M2 (minutes) (0 to enable, 1 to disable)
  // A1M3 (hour)    (0 to enable, 1 to disable)
  // A1M4 (day)     (0 to enable, 1 to disable)
  // DY/DT          (dayofweek == 1/dayofmonth == 0)
  uint8_t flags[5] = {0, 0, 0, 1, 1};

  for (int i = 0; i < 4; i++)
  {
    wake_TIME[i] = current_TIME[i];
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
    current_TIME[i] = wake_TIME[i];
  }

  Serial.println("Next wake time");
  Serial.print(wake_TIME[2], DEC);
  Serial.print(':');
  Serial.print(wake_TIME[1], DEC);
  Serial.print(':');
  Serial.print(wake_TIME[0], DEC);
  Serial.println();

  delay(5000);

  // Set the alarm time (but not yet activated)
  DS3231_set_a1(wake_TIME[0], wake_TIME[1], wake_TIME[2], 0, flags);

  // Turn the alarm on
  DS3231_set_creg(DS3231_CONTROL_INTCN | DS3231_CONTROL_A1IE);
}