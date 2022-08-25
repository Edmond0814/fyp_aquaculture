#include "Arduino.h"
#include <AltSoftSerial.h>
AltSoftSerial GSM; // RX, TX

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

void setup()
{
  GSM.begin(9600);
  Serial.begin(9600);
}

void loop()
{
  sendDataToServer();
  Serial.println("This is the line");
  delay(10000);
}

void sendDataToServer()
{
  String getURL = createGetURL();
  uint8_t serverTerm = 0;

  sendGSM("AT+SAPBR=3,1,\"APN\",\"tunetalk\"");
  sendGSM("AT+SAPBR=1,1", 3000);
  sendGSM("AT+SAPBR=2,1", 3000);
  sendGSM("AT+HTTPINIT");
  sendGSM("AT+HTTPPARA=\"CID\",1");

  GSM.println(getURL);
  while (GSM.available())
  {
    GSM.read();
  }
  delay(500);

  sendGSM("AT+HTTPACTION=0");
  GSM.read();

  //  sendGSM("AT+HTTPREAD");
  //  GSM.read();

  sendGSM("AT+HTTPTERM");
  GSM.read();

  sendGSM("AT+SAPBR=0,1");
  GSM.read();
}

String createGetURL()
{
  randomSeed(analogRead(0));
  int dioxy = random(50, 100);
  int temp = random(20, 35);

  String stringOne = "AT+HTTPPARA=\"URL\",\"http://45.127.4.18:60288/endpoint2?temp=";
  String stringTwo = "&dioxy=";
  String stringThree = "\"";
  String stringFull = stringOne + temp + stringTwo + dioxy + stringThree;

  return stringFull;
}