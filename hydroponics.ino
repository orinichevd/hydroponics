#define DEBUG_SERIAL
#define DEBUG_ETH
//#define LOG_SD

//#define BUILD_AIR
#define BUILD_SHELF1

#if defined(BUILD_SHELF1) || defined(BUILD_SHELF2)
#define BUILD_SHELF
#endif

#if defined(DEBUG_ETH) || defined(DEBUG_SERIAL)
#define DEBUG
#endif

#include <SPI.h>
#include <Ethernet2.h>

#include <avr/wdt.h>
#include <Wire.h>

//#ifdef LOG_SD
//#include <SD.h>
//#endif

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

//#ifdef LOG_SD
//const byte chipSelect = 5;
//#endif


unsigned long period = 5000;
unsigned long lastmeasuredTime = 0;
const unsigned long resetTime = 25920000;//3 days

const char server[] = "hydroponics.vo-it.ru";
const int port = 80;

#ifdef BUILD_AIR
byte mac[] = {0x90, 0xA2, 0xDA, 0x10, 0x77, 0xC8};
IPAddress ip (192, 168, 88, 12);
byte aId = 0x01;
#endif
#ifdef BUILD_SHELF1
byte mac[] = {0x90, 0xA2, 0xDA, 0x10, 0x84, 0xDE};
IPAddress ip (192, 168, 88, 14);
byte aId = 0x02;
#endif
#ifdef BUILD_SHELF2
byte mac[] = {0x90, 0xA2, 0xDA, 0x10, 0x77, 0x7A};
IPAddress ip (192, 168, 88, 13);
byte aId = 0x03;
#endif

EthernetClient client;

Sensor **sensors;
unsigned int sensorCount;

void setup()
{
#ifdef DEBUG
  Serial.begin(9600);
  while (!Serial);
  delay(1000);
  Serial.println("Serial started");
#endif
  //#ifdef LOG_SD
  //SD.begin(chipSelect);
  //writeToSD("Started");
  //#endif

#ifdef DEBUG
  period = 5000;
  Serial.println("DEBUG");
#else
  period = 60000;
#endif

#ifdef BUILD_AIR
  sensors = new Sensor *[3];
  sensorCount = 3;
  sensors[0] = new SensorMG811(A0, 2, 1);//co2
  SensorSI7021_H* airSensor = new SensorSI7021_H(0X40, 2);;
  sensors[1] = airSensor;//air t
  sensors[2] = new SensorSI7021_T(airSensor, 3);// air hum
#endif
#ifdef BUILD_SHELF1
  sensors = new Sensor *[7];
  sensorCount = 7;
  sensors[0] = new SensorBH1750(0X23, 4, 8);
  sensors[1] = new SensorBH1750(0X5C, 5, 8);
  sensors[2] = new SensorBH1750(0X23, 6, 9);
  sensors[3] = new SensorBH1750(0X5C, 7, 9);
  sensors[4] = new SensorSEN0161(A1, 8);//ph
  sensors[5] = new SensorDS18B20(A0, 9);//water t
  sensors[6] = new SensorDFR0300(A2, sensors[1], 10);//ec
#endif
#ifdef BUILD_SHELF2
  sensors = new Sensor *[7];
  sensorCount = 7;
  sensors[0] = new SensorBH1750(0X23, 4, 8);
  sensors[1] = new SensorBH1750(0X5C, 5, 8);
  sensors[2] = new SensorBH1750(0X23, 6, 9);
  sensors[3] = new SensorBH1750(0X5C, 7, 9);
  sensors[4] = new SensorSEN0161(A1, 8);//ph
  SensorDS18B20* waterTSensor = new SensorDS18B20(A0, 9);
  sensors[5] = waterTSensor;//water t
  sensors[6] = new SensorDFR0300(A2, waterTSensor, 10);//ec
#endif

  Wire.begin();

  for (int i = 0; i < sensorCount; i++)
  {
    sensors[i]->init();
  }

#if defined(DEBUG_ETH) || !defined(DEBUG)
  Ethernet.begin(mac, ip);
#endif
#ifdef DEBUG
  Serial.println("Setup done");
#endif

  wdt_enable(WDTO_8S);

}

void loop()
{
  wdt_reset();

  //reset
  if (millis() > resetTime)
  {
    while (1);
  }
  //wait for cicle
  if (millis() - lastmeasuredTime >= period)
  {
    lastmeasuredTime = millis();
  }
  else
  {
    return; //exit
  }

  String data;

  for (int i = 0; i < sensorCount; i++)
  {
    wdt_reset();
    Sensor *s = sensors[i];
    uint8_t errorCode = s->read();
    float sensorData;
    if (errorCode == S_OK)
    {
      sensorData = s->getData();
    }
    else
    {
      sensorData = 0;
    }


    addSensorInfoToData(&data, s->getSId(), s->getType(), s->getModel(), errorCode, sensorData);
  }
  #ifdef DEBUG
    Serial.println(data);
#endif

  //send data to server
#if defined(DEBUG_ETH) || !defined(DEBUG)
  sendDataToServer(&data);
#endif
}

void addSensorInfoToData(String *data, uint8_t sensorId, uint8_t type, String model, uint8_t errorCode, float value)
{
  *data = *data + String(sensorId) + ',' + String(type) + ',' + String(model) + ',' + String(errorCode) + ',' + String(value) + "\r\n";
}

void sendDataToServer(String *data)
{
  //renew connection
  client.stop();
  if (client.connect(server, port))
  {
    client.println("POST /sensorInput HTTP/1.1");
    client.println("Host: hydroponics.vo-it.ru");
    client.println("Content-Type: text/csv");
    client.print("Content-Length: ");
    client.println((*data).length());
    client.println();
    client.println((*data));
  }
  else
  {
#ifdef DEBUG
    Serial.println("can't connect to server");
#endif
  }

  client.stop();
}

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

#ifdef LOG_SD
void writeToSD(String data)
{
  File logFile = SD.open("log", FILE_WRITE);
  if (logFile)
  {
    logFile.println(String(millis()) + " " + data);
    logFile.close();
  }
}
#endif

