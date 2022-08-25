#include <OneWire.h>
#include <DallasTemperature.h>
#include <String.h>

#define ONE_WIRE_BUS 6    // define the pin for Temperature sensor (DS18B20)
#define tempUpperLimit 50 // Change the temperature limit
#define tempLowerLimit 20

#define temperature_sensor_pin 6

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

void setup()
{
  Serial.begin(9600);     // the GPRS baud rate
  sensors.begin();

  delay(1000);
}

void loop()
{
  sense_temperature();
  // float temperature = TemperatureSensor();
  // if (temperature > tempUpperLimit)
  // {
  //   Serial.println("Temperature Alert");
  //   delay(1000);
  // }
}

// float TemperatureSensor()
// {
//   Serial.print("Requesting temperature...");
//   sensors.requestTemperatures();
//   Serial.println("Done");
//   float t = sensors.getTempCByIndex(0);
//   delay(100);

//   Serial.print("Temperature = ");
//   Serial.print(t);
//   Serial.println(" °C");

//   return t;
// }

void sense_temperature(){

  float avgValue; // Store the average value of the sensor feedback
  const int buf_length = 10;
  float buf[buf_length];

  for (int i = 0; i < 10; i++) // Get 10 sample value from the sensor for smooth the value
  {
    sensors.requestTemperatures();
    buf[i] = sensors.getTempCByIndex(0);
    delay(100);
  }
  sort_array(buf, sizeof(buf)/sizeof(buf[0]));
  avgValue = get_avg_value(buf, sizeof(buf) / sizeof(buf[0]));

  Serial.print("temperature value:");
  Serial.print(avgValue);
  Serial.println(" ");
}

void sort_array(float *array_value, size_t array_size)
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

float get_avg_value(float *array_value, size_t array_size)
{
  float avgValue = 0;
  for (int i = 2; i < 8; i++) // take the average value of 6 center sample
    avgValue += array_value[i];
  avgValue = (float)avgValue / 6; // convert the analog into millivolt

  return avgValue;
}