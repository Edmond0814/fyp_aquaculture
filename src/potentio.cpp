#include <Arduino.h>

uint8_t pwm = 2, dir = 3;
int potentiometer_pin = A0;
unsigned long adcValue = 0;
unsigned long long max_adc = 500, min_adc = 0;

const int buf_length = 10;
int buf[buf_length];

void setup()
{
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
}

// the loop routine runs over and over again forever:
void loop()
{

  while (adcValue <= max_adc)
  {
    // read the input on analog pin 0:
    read_adc;

    analogRead(potentiometer_pin);

    digitalWrite(dir, LOW);      // rotate anticlockwise to drop the sensor
    int pwm_value = 80;          // make sure in the range of 0 to 255
    analogWrite(pwm, pwm_value); // Send PWM to output pin
  }

  Serial.println("Reached the bottom");
  analogWrite(pwm, 0);

  // sensor cycle starts
  delay(5000);
  Serial.println("Sensor cycle finished");

  while (adcValue >= min_adc)
  {
    analogRead(potentiometer_pin);

    // read the input on analog pin 0:
    read_adc;

    digitalWrite(dir, HIGH);     // rotate anticlockwise to drop the sensor
    int pwm_value = 80;          // make sure in the range of 0 to 255
    analogWrite(pwm, pwm_value); // Send PWM to output pin
  }

  Serial.println("Reached the top!");
  analogWrite(pwm, 0);
}

void read_adc()
{
  unsigned long int avgValue; // Store the average value of the sensor feedback

  for (int i = 0; i < 10; i++) // Get 10 sample value from the sensor for smooth the value
  {
    buf[i] = analogRead(potentiometer_pin);
    delay(10);
  }
  sort_array(buf, buf_length);
  avgValue = 0;
  for (int i = 2; i < 8; i++) // take the average value of 6 center sample
    avgValue += buf[i];
  adcValue = (float)avgValue / 6; // convert the analog into millivolt
  Serial.print("ADC value:");
  Serial.print(adcValue);
  Serial.println(" ");
}

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