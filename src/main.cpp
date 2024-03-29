#include "Arduino.h"
#include <avr/sleep.h>
#include <Wire.h>
#include <ds3231.h>
#include <AltSoftSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <String.h>
#include <Alarm.h>

#define pwm_value 200 // make sure in the range of 0 to 255 for motor
unsigned long long max_adc = 440, min_adc = 290;

// Set wake up intervals using Second, Minute, Hour and Day
// if alarm wake for every hour, set wake_intervals[4] = {0, 0, 1, 0}
// if alarm wake for every 15 minutes, set wake_intervals[4] = {0, 15, 0, 0}
uint8_t wake_intervals[4] = {0, 15, 0, 0};

// Set wake up offset using Minute and Second
// To wake every 15 minutes start from 12:05, set wake_intervals[4] = {0, 0, 15, 0}, wake_offset = {0, 5}
// will wake 12:20, 12:35, 12:50, 1:05, 1:20
uint8_t wake_offset[2] = {0, 0};

// Set time tolerance which if the time that Arduino wakes up doesn't match the time, it will not initiate a new cycle
// Using minutes
uint8_t tolerance_minutes = 3;

#define potentiometer_pin A0
#define wakePin 2 // when low, makes 328P wake up, must be an interrupt pin (2 or 3 on ATMEGA328P)
#define pwm 3
#define dir 4
#define ONE_WIRE_BUS 6 // define the pin for Temperature sensor (DS18B20)

#define tempUpperLimit 50 // Change the temperature limit
#define tempLowerLimit 20

unsigned long adcValue = 0;

// Setup a softserial instance
AltSoftSerial GSM;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// Standard setup( ) function
void setup()
{
  Serial.begin(9600);
  GSM.begin(9600); /* Define baud rate for software serial communication */
  Wire.begin();

  // Keep pins high until we ground them
  pinMode(wakePin, INPUT_PULLUP);
  pinMode(dir, OUTPUT);

  // Clear the current alarm (puts DS3231 INT high)
  DS3231_init(DS3231_CONTROL_INTCN);
  DS3231_clear_a1f();

  getCurrentTime();

  Serial.println("Setup completed.");
}

void motor_motion(uint8_t motor_dir = 1)
{
  // deploy sensor when motor_dir = 1 and rectract sensor when motor_dir = 9

  if (motor_dir == 1)
  {
    while (adcValue >= min_adc)
    {
      analogRead(potentiometer_pin);

      // read the input on analog pin 0:
      adcValue = analogRead(potentiometer_pin);

      digitalWrite(dir, LOW);      // rotate anticlockwise to drop the sensor
      analogWrite(pwm, pwm_value); // Send PWM to output pin
    }

    Serial.println("Reached the bottom");
    analogWrite(pwm, 0);
  }
  else
  {
    while (adcValue <= max_adc)
    {
      analogRead(potentiometer_pin);

      // read the input on analog pin 0:
      adcValue = analogRead(potentiometer_pin);

      digitalWrite(dir, HIGH);     // rotate anticlockwise to drop the sensor
      analogWrite(pwm, pwm_value); // Send PWM to output pin
    }

    Serial.println("Reached the top!");
    analogWrite(pwm, 0);
  }
}

void sort_array(float *array_value, size_t array_size)
{

  uint8_t temp;

  for (uint8_t i = 0; i < array_size - 1; i++) // sort the analog from small to large
  {
    for (uint8_t j = i + 1; j < array_size; j++)
    {
      if (array_value[i] > array_value[j])
      {
        temp = array_value[i];
        array_value[i] = array_value[j];
        array_value[j] = temp;
      }
    }
  }

  return;
}

float get_avg_value(float *array_value, size_t array_size)
{
  float avgValue = 0;
  for (int i = 2; i < 8; i++) // take the average value of 6 center sample
    avgValue += array_value[i];
  avgValue = (float)avgValue / 6; // convert the analog into millivolt

  return avgValue;
}

float sense_temperature()
{

  float avgValue; // Store the average value of the sensor feedback
  const int buf_length = 10;
  float buf[buf_length];

  sensors.requestTemperatures(); // read data once to remove residual voltage from previous analog readings

  for (int i = 0; i < 10; i++) // Get 10 sample value from the sensor for smooth the value
  {
    buf[i] = sensors.getTempCByIndex(0);
    delay(100);
  }
  sort_array(buf, sizeof(buf) / sizeof(buf[0]));
  avgValue = get_avg_value(buf, sizeof(buf) / sizeof(buf[0]));

  Serial.print("temperature value:");
  Serial.print(avgValue);
  Serial.println(" ");

  return avgValue;
}

void sensor_cycle(float *temperature_value)
{
  *temperature_value = sense_temperature();

  // Save to sd card code below:
}

String createGetURL(float temperature_value)
{
  randomSeed(analogRead(0));
  // int dioxy = random(50, 100);
  int dioxy = 0;

  String stringOne = "AT+HTTPPARA=\"URL\",\"http://45.127.4.18:60288/endpoint2?temp=";
  String stringTwo = "&dioxy=";
  String stringThree = "\"";
  String stringFull = stringOne + temperature_value + stringTwo + dioxy + stringThree;

  return stringFull;
}

String createThingSpeakURL(float temperature_value)
{
  randomSeed(analogRead(0));
  int dioxy = random(50, 100);
  // int temp = random(20, 35);

  String stringOne = "AT+HTTPPARA=\"URL\",\"http://api.thingspeak.com/update?api_key=LMDKI99A62C3F77X&field1=";
  String stringTwo = "&field2=";
  String stringThree = "\"";
  String stringFull = stringOne + temperature_value + stringTwo + dioxy + stringThree;

  return stringFull;
}

void sendGSM(const char *msg, int waitMs = 500)
{
  GSM.println(msg);
  while (GSM.available())
  {
    Serial.write(char(GSM.read())); /* Print character received on to the serial monitor */
    // Serial.println(GSM.read());
  }
  delay(waitMs);
}

void sendDataToServer(float temperature_value)
{
  String getURL = createGetURL(temperature_value);
  String ThingSpeak = createThingSpeakURL(temperature_value);

  sendGSM("AT+SAPBR=3,1,\"APN\",\"yoodo\"");
  sendGSM("AT+SAPBR=1,1", 3000);
  sendGSM("AT+SAPBR=2,1", 3000);
  sendGSM("AT+HTTPINIT");
  sendGSM("AT+HTTPPARA=\"CID\",1");

  GSM.println(getURL);
  while (GSM.available())
  {
    GSM.read();
  }
  delay(2000);

  sendGSM("AT+HTTPACTION=0");
  GSM.read();
  delay(2000);

  GSM.println(ThingSpeak);
  while (GSM.available())
  {
    GSM.read();
  }
  delay(2000);

  sendGSM("AT+HTTPACTION=0");
  GSM.read();
  delay(2000);

  sendGSM("AT+HTTPACTION=0");
  GSM.read();

  sendGSM("AT+HTTPTERM");
  GSM.read();

  sendGSM("AT+SAPBR=0,1");
  GSM.read();
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

void arduino_sleep()
{
  // Set the DS3231 alarm to wake up in X seconds
  setNextAlarm(wake_intervals, wake_offset);

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

void loop()
{
  float temperature_value;

  if (check_time(wake_intervals, wake_offset, tolerance_minutes)){
    Serial.println("Within time tolerance");
    // Activate motor go down
    motor_motion(1);

    // sensor cycle code
    sensor_cycle(&temperature_value);
    // randomSeed(analogRead(A2));
    // temperature_value = random(25,40);
    sendDataToServer(temperature_value);
  }

  // Activate motor go up
  motor_motion(0);

  arduino_sleep();
}