//#define LOG_SERIAL
//#define ETH_OFF
//#define LOG_SD
//#define DEBUG

#define BUILD_AIR
//#define BUILD_SHELF_TWO

#ifdef LOG_ENABLED
#include <SD.h>
#endif

#include <SPI.h>
#include <Ethernet2.h>

#include <avr/wdt.h>

#include <Wire.h>
#include "log.h"
#include "pinMap.h"

#ifdef BUILD_AIR
#include "MG811.h"
#include "SI7021.h"
#endif

#if (defined(BUILD_SHELF_ONE) || defined(BUILD_SHELF_TWO))
#include "SEN0161.h"
#include "DFR0300.h"
#include "BH1750.h"
#endif

#include "Sensor.h"

//#ifdef DEBUG
//unsigned long period = 5000;
//#else
unsigned long period = 30000;
//#endif

unsigned long lastmeasuredTime = 0;
const unsigned long resetTime = 600000;//1 hour

const char server[] = "hydroponics.vo-it.ru";
const int port = 80;
uint8_t failCount = 0;
const int maxFailCount = 3;

IPAddress google_dns (8, 8, 8, 8);

#ifdef BUILD_AIR
byte mac[] = {0x90, 0xA2, 0xDA, 0x10, 0x77, 0xC8};
IPAddress ip (192, 168, 88, 12);
Sensor *sensors[3];
const unsigned int sensorCount = 3;
#endif

#if defined(BUILD_SHELF_ONE)
byte mac[] = {0x90, 0xA2, 0xDA, 0x10, 0x84, 0xDE};
IPAddress ip (192, 168, 88, 14);
Sensor *sensors[5];
const unsigned int sensorCount = 5;
#endif

#if defined(BUILD_SHELF_TWO)
byte mac[] = {0x90, 0xA2, 0xDA, 0x10, 0x77, 0x7A};
IPAddress ip (192, 168, 88, 13);
Sensor *sensors[5];
const unsigned int sensorCount = 5;
#endif

#ifndef ETH_OFF
EthernetClient client;
#endif

Logger logWriter(CS_PIN);

void setup()
{
#ifdef DEBUG
  logWriter.init();
#else
  Serial.begin(9600);
#endif

  delay(5000);
  wdt_enable(WDTO_1S);
  
#ifdef BUILD_AIR
  sensors[0] = new SensorMG811(MGH11_ANALOG_PIN, MGH11_DIGITAL_PIN, 1);//co2
  SensorSI7021_H* airSensor = new SensorSI7021_H(SI7021_I2C_ADDRESS, 2);
  sensors[1] = airSensor;//air t
  sensors[2] = new SensorSI7021_T(airSensor, 3);// air hum
#endif
#ifdef BUILD_SHELF_ONE
  sensors[0] = new SensorBH1750(BH1750_I2C_ADDRESS_1, BH1750_BUS_SELECTION_PIN1, BH1750_BUS_SELECTION_PIN2, 4);
  sensors[1] = new SensorBH1750(BH1750_I2C_ADDRESS_2, BH1750_BUS_SELECTION_PIN1, BH1750_BUS_SELECTION_PIN2, 5);
  //sensors[2] = new SensorBH1750(BH1750_I2C_ADDRESS_1, BH1750_BUS_SELECTION_PIN2, BH1750_BUS_SELECTION_PIN1, 6);
  //sensors[3] = new SensorBH1750(BH1750_I2C_ADDRESS_2, BH1750_BUS_SELECTION_PIN2, BH1750_BUS_SELECTION_PIN1, 7);
  sensors[2] = new SensorSEN0161(SEN0161_ANALOG_PIN, 8);//ph
  SensorDS18B20* waterTSensor = new SensorDS18B20(DS18B20_ANALOG_PIN, 9);//water t
  sensors[3] = waterTSensor;//water t
  sensors[4] = new SensorDFR0300(DFR0300_ANALOG_PIN, waterTSensor, 10);//ec
#endif
#ifdef BUILD_SHELF_TWO
  sensors[0] = new SensorBH1750(BH1750_I2C_ADDRESS_1, BH1750_BUS_SELECTION_PIN1, BH1750_BUS_SELECTION_PIN2, 11);
  sensors[1] = new SensorBH1750(BH1750_I2C_ADDRESS_2, BH1750_BUS_SELECTION_PIN1, BH1750_BUS_SELECTION_PIN2, 12);
  //sensors[2] = new SensorBH1750(BH1750_I2C_ADDRESS_1, BH1750_BUS_SELECTION_PIN2, BH1750_BUS_SELECTION_PIN1, 13);
  //sensors[3] = new SensorBH1750(BH1750_I2C_ADDRESS_2, BH1750_BUS_SELECTION_PIN2, BH1750_BUS_SELECTION_PIN1, 14);
  sensors[2] = new SensorSEN0161(SEN0161_ANALOG_PIN, 15);//ph
  SensorDS18B20* waterTSensor = new SensorDS18B20(DS18B20_ANALOG_PIN, 16);
  sensors[3] = waterTSensor;//water t
  sensors[4] = new SensorDFR0300(DFR0300_ANALOG_PIN, waterTSensor, 17);//ec
#endif
  wdt_reset();
  Wire.begin();
  wdt_reset();
  delay(500);
  wdt_reset();
  delay(500);

  for (int i = 0; i < sensorCount; i++)
  {
    wdt_reset();
    sensors[i]->init();
  }

  logWriter.logData("inited");

  Ethernet.begin(mac, ip, google_dns);
  lastmeasuredTime = millis();
  
}

void loop()
{
  wdt_reset();
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
    char buf[25];
    wdt_reset();
    Sensor *s = sensors[i];
    uint8_t errorCode = s->read();
    wdt_reset();
    float sensorData = errorCode == S_OK ? s->getData() : 0.0;
    addSensorInfoToData(buf, s, errorCode, sensorData);
    wdt_reset();
    strcat(data, buf);
  }
  logWriter.logData(data);

  //send data to server
#ifndef ETH_OFF
  wdt_reset();
  sendDataToServer(data);
#endif
  resetEth();
  while (1);
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
    wdt_reset();
  }
  else
  {
    failCount++;
    logWriter.logData("can't sent data");
  }
  client.stop();
}
#endif



void resetEth()
{
  uint16_t _addr = 0x00;
  uint8_t _cb = 0x04;
  uint8_t _data = 0x80;
  SPISettings wiznet_SPI_settings(800000, MSBFIRST, SPI_MODE0);
  SPI.beginTransaction(wiznet_SPI_settings);
  digitalWrite(10, LOW);
  SPI.transfer(_addr >> 8);
  SPI.transfer(_addr & 0xFF);
  SPI.transfer(_cb);
  SPI.transfer(_data);
  digitalWrite(10, HIGH);
  SPI.endTransaction();
}

