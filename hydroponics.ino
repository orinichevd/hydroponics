//#define LOG_SERIAL
//#define ETH_OFF
//#define LOG_SD
//#define DEBUG

#if  defined(LOG_SD) || defined(LOG_SERIAL)
#define LOG_ENABLED
#endif

//#define BUILD_AIR
#define BUILD_SHELF2

#if defined(BUILD_SHELF1) || defined(BUILD_SHELF2)
#define BUILD_SHELF
#endif

#include <SPI.h>
#include <Ethernet2.h>

#include <avr/wdt.h>
#include <Wire.h>
#include "log.h"
#include "pinMap.h"

#ifdef LOG_ENABLED
#include <SD.h>
#endif

#ifdef BUILD_AIR
#include "MG811.h"
#include "SI7021.h"
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

IPAddress google_dns (8, 8, 8, 8);

#ifdef BUILD_AIR
byte mac[] = {0x90, 0xA2, 0xDA, 0x10, 0x77, 0xC8};
IPAddress ip (192, 168, 88, 12);
Sensor *sensors[3];
const unsigned int sensorCount = 3;
#endif

#ifdef BUILD_SHELF1
byte mac[] = {0x90, 0xA2, 0xDA, 0x10, 0x84, 0xDE};
IPAddress ip (192, 168, 88, 14);
Sensor *sensors[5];
const unsigned int sensorCount = 5;
#endif

#ifdef BUILD_SHELF2
byte mac[] = {0x90, 0xA2, 0xDA, 0x10, 0x77, 0x7A};
IPAddress ip (192, 168, 88, 13);
Sensor *sensors[5];
const unsigned int sensorCount = 5;
#endif

#ifndef ETH_OFF
EthernetClient client;
#endif

//#ifdef LOG_ENABLED
Logger logWriter(CS_PIN);
//#endif

void setup()
{
#ifdef DEBUG
  logWriter.init();
  period = 5000;
#else
  period = 60000;
#endif

#ifdef BUILD_AIR
  sensors[0] = new SensorMG811(MGH11_ANALOG_PIN, MGH11_DIGITAL_PIN, 1);//co2
  SensorSI7021_H* airSensor = new SensorSI7021_H(SI7021_I2C_ADDRESS, 2);;
  sensors[1] = airSensor;//air t
  sensors[2] = new SensorSI7021_T(airSensor, 3);// air hum
#endif
#ifdef BUILD_SHELF1
  sensors[0] = new SensorBH1750(BH1750_I2C_ADDRESS_1, BH1750_BUS_SELECTION_PIN1, BH1750_BUS_SELECTION_PIN2, 4);
  sensors[1] = new SensorBH1750(BH1750_I2C_ADDRESS_2, BH1750_BUS_SELECTION_PIN1, BH1750_BUS_SELECTION_PIN2, 5);
  //sensors[2] = new SensorBH1750(BH1750_I2C_ADDRESS_1, BH1750_BUS_SELECTION_PIN2, BH1750_BUS_SELECTION_PIN1, 6);
  //sensors[3] = new SensorBH1750(BH1750_I2C_ADDRESS_2, BH1750_BUS_SELECTION_PIN2, BH1750_BUS_SELECTION_PIN1, 7);   
  sensors[2] = new SensorSEN0161(SEN0161_ANALOG_PIN, 8);//ph
  SensorDS18B20* waterTSensor = new SensorDS18B20(DS18B20_ANALOG_PIN, 9);//water t
  sensors[3] = waterTSensor;//water t
  sensors[4] = new SensorDFR0300(DFR0300_ANALOG_PIN, waterTSensor, 10);//ec
#endif
#ifdef BUILD_SHELF2
  sensors[0] = new SensorBH1750(BH1750_I2C_ADDRESS_1, BH1750_BUS_SELECTION_PIN1, BH1750_BUS_SELECTION_PIN2, 11);
  sensors[1] = new SensorBH1750(BH1750_I2C_ADDRESS_2, BH1750_BUS_SELECTION_PIN1, BH1750_BUS_SELECTION_PIN2, 12);
  //sensors[2] = new SensorBH1750(BH1750_I2C_ADDRESS_1, BH1750_BUS_SELECTION_PIN2, BH1750_BUS_SELECTION_PIN1, 13);
  //sensors[3] = new SensorBH1750(BH1750_I2C_ADDRESS_2, BH1750_BUS_SELECTION_PIN2, BH1750_BUS_SELECTION_PIN1, 14);
  sensors[2] = new SensorSEN0161(SEN0161_ANALOG_PIN, 15);//ph
  SensorDS18B20* waterTSensor = new SensorDS18B20(DS18B20_ANALOG_PIN, 16);
  sensors[3] = waterTSensor;//water t
  sensors[4] = new SensorDFR0300(DFR0300_ANALOG_PIN, waterTSensor, 17);//ec
#endif

  Wire.begin();
  delay(1000);
  for (int i = 0; i < sensorCount; i++)
  {
    sensors[i]->init();
  }
  logWriter.logData("inited");
  
#ifndef ETH_OFF
  Ethernet.begin(mac,ip, google_dns);
#endif
#ifndef DEBUG
  wdt_enable(WDTO_8S);
#endif
  lastmeasuredTime = millis();
}

void loop()
{
  wdt_reset();

  //reset
#ifndef DEBUG
  if (millis() > resetTime)
  {
    while (1);
  }
#endif
  //wait for cicle
  if (millis() - lastmeasuredTime <= period)
  {
    return;
  }
  lastmeasuredTime = millis(); 
  char data[500];
  data[0] = '\0';

  for (int i = 0; i < sensorCount; i++)
  {
    //logWriter.logData("s");
    char buf[25];
    wdt_reset();
    Sensor *s = sensors[i];
    uint8_t errorCode = s->read();
    float sensorData = errorCode == S_OK ? s->getData() : 0.0;
    addSensorInfoToData(buf, s, errorCode, sensorData);
    strcat(data, buf);
  }
logWriter.logData(data);

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


