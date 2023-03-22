#ifndef Alarm_h
#define Alarm_h

#include <stdlib.h>
#if ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#include <wiring.h>
#endif

// These defs cause trouble on some versions of Arduino
#undef round

// Use the system yield() whenever possoible, since some platforms require it for housekeeping, especially
// ESP8266
#if (defined(ARDUINO) && ARDUINO >= 155) || defined(ESP8266)
#define YIELD yield();
#else
#define YIELD
#endif

void getCurrentTime();
boolean check_time(uint8_t wake_intervals[4], uint8_t wake_offset[2], uint8_t wake_tolerance);
void setNextAlarm(uint8_t wake_intervals[4], uint8_t wake_offset[2]);

#endif