#include <Arduino.h>
#include <SPI.h>               //SPI bus library
#include <SD.h>                //SD card library
#include <SoftwareSerial.h>    //GSM and GPRS library
#include <String.h>            //String library
#include <Wire.h>              //I2C library
#include <Time.h>              //RTC add-on library
#include <DS1307RTC.h>         //RTC library
#include <LCD.h>               //LCD library
#include <LiquidCrystal_I2C.h> //LCD I2C library
#include <ArduinoJson.h>       //JSON  library

#define SensorPin 0 // the pH meter Analog output is connected with the Arduino’s Analog
#define dir 2
#define pwm 3

SoftwareSerial SIM900(7, 8);                                   // Create object named SIM900 of the class SoftwareSerial with digital output pin of 7 and 8
LiquidCrystal_I2C lcd(0x27,2,1,0,4,5,6,7,3,POSITIVE); // Create liquid crystal object called 'lcd'
tmElements_t tm;

int a = 0;
char *strings[16]; // an array of pointers to the pieces of the above array after strtok()
char *ptr = NULL;
int timer = 30; // 30 mins

const char *monthName[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

unsigned long int avgValue; // Store the average value of the sensor feedback
float b;
const int buf_length = 10;
int buf[buf_length];

File myFile;

// change this to match your SD shield or module;
const int chipSelect = 10;

void sort_array(int array_value[], int array_size)
{

  int temp;

  for (int i = 0; i < array_size - 1; i++) // sort the analog from small to large
  {
    for (int j = i + 1; j < array_size; j++)
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

void get_ph_value()
{
  unsigned long int avgValue; // Store the average value of the sensor feedback


  for (int i = 0; i < 10; i++) // Get 10 sample value from the sensor for smooth the value
  {
    buf[i] = analogRead(SensorPin);
    delay(10);
  }
  sort_array(buf, buf_length);
  avgValue = 0;
  for (int i = 2; i < 8; i++) // take the average value of 6 center sample
    avgValue += buf[i];
  float phValue = (float)avgValue * 5.0 / 1024 / 6; // convert the analog into millivolt
  phValue = 3.5 * phValue;                          // convert the millivolt into pH value
  Serial.print("    pH:");
  Serial.print(phValue, 2);
  Serial.println(" ");

  lcd.clear(); // Clear any text from the screen
  lcd.setCursor(1, 0);
  lcd.print("Ph Value"); // Print 'Ph Value String'
  delay(500);

  lcd.setCursor(1, 1);
  lcd.print(phValue); // Print 'ph value of solution'
  delay(500);

  digitalWrite(13, HIGH);
  delay(800);
  digitalWrite(13, LOW);
}

void setup()
{
  // put your setup code here, to run once:
  get_ph_value();
}

void loop()
{
  // put your main code here, to run repeatedly:
}
