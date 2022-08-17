// Do not remove the include below
#include "Arduino_Sleep_DS3231_Wakeup.h"

/*
 Low Power SLEEP modes for Arduino UNO/Nano
 using Atmel328P microcontroller chip.

 For full details see my video #115
 at https://www.youtube.com/ralphbacon
 (Direct link to video: https://TBA)

This sketch will wake up out of Deep Sleep when the RTC alarm goes off

 All details can be found at https://github.com/ralphbacon

 */
#include "Arduino.h"
#include <avr/sleep.h>
#include <wire.h>
#include <DS3231.h>
#include <SoftwareSerial.h>
#include <String.h>

#define wakePin 3  // when low, makes 328P wake up, must be an interrupt pin (2 or 3 on ATMEGA328P)

// Set wake up intervals using Second, Minute, Hour and Day
// wake every hour = {0, 0, 1, 0}
// wake every 15 minutes = {0, 15, 0, 0}
uint8_t wake_intervals[4] = {0, 10, 0, 0};

// Set wake up offset using Minute and Second
// To wake every 15 minutes start from 12:05, set wake_intervals[4] = {0, 0, 15, 0}, wake_offset = {0, 5}
// will wake 12:20, 12:35, 12:50, 1:05, 1:20
uint8_t wake_offset[2] = {0, 0};

// DS3231 alarm time
uint8_t wake_TIME[4] = {0, 0, 0, 0};

uint8_t previous_wake_TIME[4] = {0, 0, 0, 0};

#define BUFF_MAX 256

struct ts t;

enum _parseState
{
  PS_DETECT_MSG_TYPE,

  PS_IGNORING_COMMAND_ECHO,

  PS_HTTPACTION_TYPE,
  PS_HTTPACTION_RESULT,
  PS_HTTPACTION_LENGTH,

  PS_HTTPREAD_LENGTH,
  PS_HTTPREAD_CONTENT
};

byte parseState = PS_DETECT_MSG_TYPE;
char buffer[80];
byte pos = 0;

int contentLength = 0;

SoftwareSerial GSM(8, 9);

// Standard setup( ) function
void setup()
{
  Serial.begin(9600);
  GSM.begin(9600); /* Define baud rate for software serial communication */
  Wire.begin();
  while (!Serial)
    ; // wait for serial
  delay(200);

  // Keep pins high until we ground them
  pinMode(wakePin, INPUT_PULLUP);

  // Clear the current alarm (puts DS3231 INT high)
  DS3231_init(DS3231_CONTROL_INTCN);
  DS3231_clear_a1f();

  DS3231_get(&t);
  previous_wake_TIME[0] = t.sec;
  previous_wake_TIME[1] = t.min;
  previous_wake_TIME[2] = t.hour;


  Serial.println("Setup completed.");
}

// The loop blinks an LED when not in sleep mode
void loop()
{
  static uint8_t oldSec = 99;
  char buff[BUFF_MAX];

  

  
    // Get the time
    DS3231_get(&t);

    // If the seconds has changed, display the (new) time
    if (t.sec != oldSec)
    {
      // display current time
      snprintf(buff, BUFF_MAX, "%d.%02d.%02d %02d:%02d:%02d\n", t.year,
               t.mon, t.mday, t.hour, t.min, t.sec);
      Serial.print(buff);
      oldSec = t.sec;
    }
}

// When wakePin is brought LOW this interrupt is triggered FIRST (even in PWR_DOWN sleep)
void sleepISR()
{
  // Prevent sleep mode, so we don't enter it again, except deliberately, by code
  sleep_disable();

  // Detach the interrupt that brought us out of sleep
  detachInterrupt(digitalPinToInterrupt(wakePin));

  // Now we continue running the main Loop() just after we went to sleep
}

// Set the next alarm
void setNextAlarm(void)
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

  // Set the alarm time (but not yet activated)
  DS3231_set_a1(wake_TIME[0], wake_TIME[1], wake_TIME[2], 0, flags);

  // Turn the alarm on
  DS3231_set_creg(DS3231_CONTROL_INTCN | DS3231_CONTROL_A1IE);
}

void arduino_sleep(){
  // Set the DS3231 alarm to wake up in X seconds
  setNextAlarm();

  // Disable the ADC (Analog to digital converter, pins A0 [14] to A5 [19])
  static byte prevADCSRA = ADCSRA;
  ADCSRA = 0;

  /* Set the type of sleep mode we want. Can be one of (in order of power saving):

   SLEEP_MODE_IDLE (Timer 0 will wake up every millisecond to keep millis running)
   SLEEP_MODE_ADC
   SLEEP_MODE_PWR_SAVE (TIMER 2 keeps running)
   SLEEP_MODE_EXT_STANDBY
   SLEEP_MODE_STANDBY (Oscillator keeps running, makes for faster wake-up)
   SLEEP_MODE_PWR_DOWN (Deep sleep)
   */
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();

  // Turn of Brown Out Detection (low voltage)
  // Thanks to Nick Gammon for how to do this (temporarily) in software rather than
  // permanently using an avrdude command line.
  //
  // Note: Microchip state: BODS and BODSE only available for picoPower devices ATmega48PA/88PA/168PA/328P
  //
  // BODS must be set to one and BODSE must be set to zero within four clock cycles. This sets
  // the MCU Control Register (MCUCR)
  MCUCR = bit(BODS) | bit(BODSE);

  // The BODS bit is automatically cleared after three clock cycles so we better get on with it
  MCUCR = bit(BODS);

  // Ensure we can wake up again by first disabling interupts (temporarily) so
  // the wakeISR does not run before we are asleep and then prevent interrupts,
  // and then defining the ISR (Interrupt Service Routine) to run when poked awake
  noInterrupts();
  attachInterrupt(digitalPinToInterrupt(wakePin), sleepISR, LOW);

  // Send a message just to show we are about to sleep
  Serial.println("Good night!");
  Serial.flush();

  // Allow interrupts now
  interrupts();

  // And enter sleep mode as set above
  sleep_cpu();

  // --------------------------------------------------------
  // �Controller is now asleep until woken up by an interrupt
  // --------------------------------------------------------

  // Wakes up at this point when wakePin is brought LOW - interrupt routine is run first
  Serial.println("I'm awake!");

  // Clear existing alarm so int pin goes high again
  DS3231_clear_a1f();

  // Re-enable ADC if it was previously running
  ADCSRA = prevADCSRA;

  return;
}

void resetBuffer()
{
  memset(buffer, 0, sizeof(buffer));
  pos = 0;
}

void sendGSM(const char *msg, int waitMs = 500)
{
  GSM.println(msg);
  delay(waitMs);
  while (GSM.available())
  {
    parseATText(GSM.read());
  }
}

void send_data_to_server(){
  int dioxy = random(50, 100);
  int temp = random (20, 35);

  String stringone = "AT+HTTPPARA=\"URL\",\"http://45.127.4.18:60288/endpoint2?temp=";
  String stringtwo = "&dioxy=";
  String stringthree = "\"";
  String stringfull = stringone + temp + stringtwo + dioxy + stringthree;

  return;
}

void parseATText(byte b)
{

  buffer[pos++] = b;

  if (pos >= sizeof(buffer))
    resetBuffer(); // just to be safe

  /*
   // Detailed debugging
   Serial.println();
   Serial.print("state = ");
   Serial.println(state);
   Serial.print("b = ");
   Serial.println(b);
   Serial.print("pos = ");
   Serial.println(pos);
   Serial.print("buffer = ");
   Serial.println(buffer);*/

  switch (parseState)
  {
  case PS_DETECT_MSG_TYPE:
  {
    if (b == '\n')
      resetBuffer();
    else
    {
      if (pos == 3 && strcmp(buffer, "AT+") == 0)
      {
        parseState = PS_IGNORING_COMMAND_ECHO;
      }
      else if (b == ':')
      {
        // Serial.print("Checking message type: ");
        // Serial.println(buffer);

        if (strcmp(buffer, "+HTTPACTION:") == 0)
        {
          Serial.println("Received HTTPACTION");
          parseState = PS_HTTPACTION_TYPE;
        }
        else if (strcmp(buffer, "+HTTPREAD:") == 0)
        {
          Serial.println("Received HTTPREAD");
          parseState = PS_HTTPREAD_LENGTH;
        }
        resetBuffer();
      }
    }
  }
  break;

  case PS_IGNORING_COMMAND_ECHO:
  {
    if (b == '\n')
    {
      Serial.print("Ignoring echo: ");
      Serial.println(buffer);
      parseState = PS_DETECT_MSG_TYPE;
      resetBuffer();
    }
  }
  break;

  case PS_HTTPACTION_TYPE:
  {
    if (b == ',')
    {
      Serial.print("HTTPACTION type is ");
      Serial.println(buffer);
      parseState = PS_HTTPACTION_RESULT;
      resetBuffer();
    }
  }
  break;

  case PS_HTTPACTION_RESULT:
  {
    if (b == ',')
    {
      Serial.print("HTTPACTION result is ");
      Serial.println(buffer);
      parseState = PS_HTTPACTION_LENGTH;
      resetBuffer();
    }
  }
  break;

  case PS_HTTPACTION_LENGTH:
  {
    if (b == '\n')
    {
      Serial.print("HTTPACTION length is ");
      Serial.println(buffer);

      // now request content
      GSM.print("AT+HTTPREAD=0,");
      GSM.println(buffer);

      parseState = PS_DETECT_MSG_TYPE;
      resetBuffer();
    }
  }
  break;

  case PS_HTTPREAD_LENGTH:
  {
    if (b == '\n')
    {
      contentLength = atoi(buffer);
      Serial.print("HTTPREAD length is ");
      Serial.println(contentLength);

      Serial.print("HTTPREAD content: ");

      parseState = PS_HTTPREAD_CONTENT;
      resetBuffer();
    }
  }
  break;

  case PS_HTTPREAD_CONTENT:
  {
    // for this demo I'm just showing the content bytes in the serial monitor
    Serial.write(b);

    contentLength--;

    if (contentLength <= 0)
    {

      // all content bytes have now been read

      parseState = PS_DETECT_MSG_TYPE;
      resetBuffer();
    }
  }
  break;
  }
}