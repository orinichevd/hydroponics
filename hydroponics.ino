//#define DEBUG_SERIAL
//#define DEBUG_ETHRNET
#define BUILD_AIR

#include <SPI.h>
#include <Ethernet2.h>
#include <Wire.h>

#include "MG811.h"
#include "DHT11.h"

#include "SEN0161.h"
#include "DFR0300.h"
//#include "BH1750.h"
#include "Sensor.h"


const char server[] = "hydroponics.vo-it.ru";
const int port = 80;

#ifdef BUILD_AIR
byte mac[] = {0x90, 0xA2, 0xDA, 0x10, 0x77, 0xC8};
byte aId = 0x01;
#endif
#ifdef BUILD_SHELF1
byte mac[] = {0x90, 0xA2, 0xDA, 0x10, 0x84, 0xDE};
byte aId = 0x02;
#endif
#ifdef BUILD_SHELF2
byte mac[] = {0x90, 0xA2, 0xDA, 0x10, 0x77, 0x7A};
byte aId = 0x03;
#endif


EthernetClient client;

Sensor **sensors = new Sensor *[3];
const unsigned int sensorCount = 3;

void setup()
{
  Serial.begin(9600);
#ifdef DEBUG_SERIAL
  while (!Serial)
  {
    ;
  }
#endif
  delay(1000);
  Serial.println("Serial started");
#ifdef BUILD_AIR
  sensors[0] = new SensorMG811(A0, 2, 1);//co2
  SensorDHT11_T* airSensor = new SensorDHT11_T(5, 2);
  sensors[1] = airSensor;//air t
  sensors[2] = new SensorDHT11_Hum(airSensor, 3);// air hum
#endif
#ifdef BUILD_SHELF1

  sensors[0] = new SensorBH1750(7, BH1750_CONTINUOUS_HIGH_RES_MODE);
  //sensors[0] = new SensorSEN0161(0, 1, 4);//ph
  //sensors[1] = new SensorDS18B20(A1, 2, 6);//water t
  //sensors[2] = new SensorDFR0300(2, sensors[1], 3, 5);//ec
#endif
  for (int i = 0; i < sensorCount; i++)
  {
    sensors[i]->init();
  }
#ifndef DEBUG_SERIAL
  if (!Ethernet.begin(mac))
  {
    Serial.println("Ethernet not started");
  }
  Serial.println("Setup done");
#endif
}

void loop()
{
  String data;

  for (int i = 0; i < sensorCount; i++)
  {
    Sensor *s = sensors[i];
    switch (s->getType())
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
    uint8_t errorCode = s->read();
    float sensorData;
    if (errorCode == S_OK)
    {
      sensorData = s->getData();
      Serial.println(sensorData);
    }
    else
    {
      sensorData = 0;
      switch (errorCode)
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
      }
    }
    addSensorInfoToData(&data, s->getSId(), s->getType(), s->getModel(), errorCode, sensorData);
  }

  //send data to server
#ifndef DEBUG_SERIAL
  sendDataToServer(&data);
#endif
  delay(60000);
}

void addSensorInfoToData(String *data, uint8_t sensorId, uint8_t type, String model, uint8_t errorCode, float value)
{
  *data = *data + String(sensorId) + ',' + String(type) + ',' + String(model) + ',' + String(errorCode) + ',' + String(value) + "\r\n";
}

void sendDataToServer(String *data)
{
  // start the Ethernet connection:
  Ethernet.maintain();
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
    Serial.println(*data);
  }
  else
  {
    Serial.println("can't connect to server");
  }
  client.stop();
}
