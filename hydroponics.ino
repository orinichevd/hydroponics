//#define ETH_OFF
//#define DEBUG
//first data received at 20-1 on friday 14
#define LOG_ENABLED


#define BUILD_AIR
//#define BUILD_SHELF1

#if defined(BUILD_SHELF1) || defined(BUILD_SHELF2)
#define BUILD_SHELF
#endif

#define CS_PIN 4

#include <SPI.h>
#include <Ethernet2.h>

#include <SD.h>
#include "log.h"

#include <avr/wdt.h>
//#include <Wire.h>

#ifdef BUILD_AIR
#include "MG811.h"
//#include "SI7021.h"
#endif

#ifdef BUILD_SHELF
#include "SEN0161.h"
#include "DFR0300.h"
#include "BH1750.h"
#endif

#include "Sensor.h"

unsigned long period = 5000;
unsigned long lastmeasuredTime = 0;
const unsigned long resetTime = 25920000;//3 days

const char server[] = "hydroponics.vo-it.ru";
const int port = 80;
uint8_t failCount = 0;
const int maxFailCount = 10;

#ifdef BUILD_AIR
byte mac[] = {0x90, 0xA2, 0xDA, 0x10, 0x77, 0xC8};
IPAddress ip (192, 168, 88, 12);
Sensor *sensors[1];
const unsigned int sensorCount = 1;
#endif

#ifdef BUILD_SHELF1
byte mac[] = {0x90, 0xA2, 0xDA, 0x10, 0x84, 0xDE};
IPAddress ip (192, 168, 88, 14);
Sensor *sensors[7];
const unsigned int sensorCount = 7;
#endif

#ifdef BUILD_SHELF2
byte mac[] = {0x90, 0xA2, 0xDA, 0x10, 0x77, 0x7A};
IPAddress ip (192, 168, 88, 13);
Sensor *sensors[7];
const unsigned int sensorCount = 7;
#endif

#ifndef ETH_OFF
EthernetClient client;
#endif

Logger logWriter(CS_PIN);

void setup()
{
#ifdef LOG_ENABLED
  logWriter.init();
#endif
#ifdef DEBUG
  period = 5000;
  logWriter.logData("STARTED IN DEBUG MODE");
#else
  period = 60000;
  logWriter.logData("STARTED IN RELEASE MODE");
#endif
  //Wire.begin();
#ifndef ETH_OFF
  Ethernet.begin(mac, ip);
#endif

#ifdef BUILD_AIR
  sensors[0] = new SensorMG811(A0, 2, 1);//co2
  //SensorSI7021_H* airSensor = new SensorSI7021_H(0X40, 2);;
  //sensors[1] = airSensor;//air t
  //sensors[2] = new SensorSI7021_T(airSensor, 3);// air hum
#endif
#ifdef BUILD_SHELF1
  sensors[0] = new SensorBH1750(0X23, 4, 8);
  sensors[1] = new SensorBH1750(0X5C, 5, 8);
  sensors[2] = new SensorBH1750(0X23, 6, 9);
  sensors[3] = new SensorBH1750(0X5C, 7, 9);
  sensors[4] = new SensorSEN0161(A1, 8);//ph
  sensors[5] = new SensorDS18B20(A0, 9);//water t
  sensors[6] = new SensorDFR0300(A2, sensors[1], 10);//ec
#endif
#ifdef BUILD_SHELF2
  sensors[0] = new SensorBH1750(0X23, 11, 8);
  sensors[1] = new SensorBH1750(0X5C, 12, 8);
  sensors[2] = new SensorBH1750(0X23, 13, 9);
  sensors[3] = new SensorBH1750(0X5C, 14, 9);
  sensors[4] = new SensorSEN0161(A1, 15);//ph
  SensorDS18B20* waterTSensor = new SensorDS18B20(A0, 16);
  sensors[5] = waterTSensor;//water t
  sensors[6] = new SensorDFR0300(A2, waterTSensor, 17);//ec
#endif

  for (int i = 0; i < sensorCount; i++)
  {
    sensors[i]->init();
  }

  wdt_enable(WDTO_8S);

}

void loop()
{
  wdt_reset();

  //reset if time to update or cant send data to server many times
  if (millis() > resetTime || failCount >= maxFailCount)
  {
    logWriter.logData("time to reset");
    while (1);
  }
  //wait for cicle
  if (millis() - lastmeasuredTime <= period)
  {
    return;
  }
  logWriter.logData("start measure");
  lastmeasuredTime = millis();

  char data[500];
  data[0] = '\0';

  for (int i = 0; i < sensorCount; i++)
  {
    char buf[25];
    wdt_reset();
    Sensor *s = sensors[i];
    uint8_t errorCode = s->read();
    float sensorData = errorCode == S_OK ? s->getData() : 0.0;
    addSensorInfoToData(buf, s, errorCode, sensorData);
    logWriter.logData(buf);
    strcat(data, buf);
  }

  //send data to server
#ifndef ETH_OFF
  sendDataToServer(data);
#endif
}

void addSensorInfoToData(char* data, Sensor* s, uint8_t errorCode, float value)
{
  int fractpart, intpart;
  intpart = trunc(value);
  fractpart = trunc((value - trunc(value)) * 100);
  sprintf(data, "%d,%d,%s,%d,%d.%d\r\n", s->getSId(), s->getType(), s->getModel(), errorCode, intpart, fractpart);
}

#ifndef ETH_OFF
void sendDataToServer(char *data)
{
  //renew connection
  client.stop();
  if (client.connect(server, port))
  {
    client.println("POST /sensorInput HTTP/1.1");
    client.println("Host: hydroponics.vo-it.ru");
    client.println("Content-Type: text/csv");
    client.print("Content-Length: ");
    client.println(strlen(data));
    client.println();
    client.println(data);
    logWriter.logData("data sent");
  }
  else
  {
    logWriter.logData("cant send to server");
    failCount++;
  }
  
}
#endif

/*void printErrorCode(uint8_t error)
{
  switch (error)
  {
    case S_OUT_OF_RANGE:
      Serial.println("out of range");
      break;
    case S_ERROR_CHECKSUM:
      Serial.println("checksum error");
      break;
    case S_ERROR_TIMEOUT:
      Serial.println("timeout error");
      break;
    case S_NOT_RECOGNIZED:
      Serial.println("cant found sensor on port");
    default:
      Serial.println("error reading");
  }
}


void printSensorType(uint8_t type)
{
  switch (type)
  {
    case S_TYPE_CO2:
      Serial.print("CO2: ");
      break;
    case S_TYPE_T_AIR:
      Serial.print("T: ");
      break;
    case S_TYPE_HUMIDITY:
      Serial.print("Hum: ");
      break;
    case S_TYPE_PH:
      Serial.print("Ph: ");
      break;
    case S_TYPE_EC:
      Serial.print("Ec: ");
      break;
    case S_TYPE_T_WATER:
      Serial.print("Water T: ");
      break;
    case S_TYPE_LIGHT:
      Serial.print("Light: ");
      break;
  }
}*/


