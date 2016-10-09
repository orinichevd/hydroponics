
//#define DEBUG_SERIAL
//#define DEBUG_ETH

//#define BUILD_AIR
#define BUILD_SHELF2

#if defined(BUILD_SHELF1) || defined(BUILD_SHELF2)
#define BUILD_SHELF
#endif

#if defined(DEBUG_ETH) || defined(DEBUG_SERIAL)

#define DEBUG

#endif


#include <SPI.h>
#include <Ethernet2.h>
#include <Wire.h>
#include <avr/wdt.h>

#ifdef BUILD_AIR
#include "MG811.h"
#include "DHT11.h"
#endif

#ifdef BUILD_SHELF
#include "SEN0161.h"
#include "DFR0300.h"
#include "BH1750.h"
#endif

#include "Sensor.h"

unsigned long period = 5000;
const unsigned long resetTime = 25920000;

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

Sensor **sensors;
unsigned int sensorCount;


void setup()
{


  Serial.begin(9600);

#ifdef DEBUG_SERIAL
  while (!Serial);
  delay(1000);
  Serial.println("Serial started");
#endif

#ifdef DEBUG
  period = 5000;
  Serial.println("DEBUG");
#else
  period = 60000;
  Serial.println("RELEASE");
#endif

#ifdef BUILD_AIR
  sensors = new Sensor *[3];
  sensorCount = 3;
  sensors[0] = new SensorMG811(A0, 2, 1);//co2
  SensorDHT11_T* airSensor = new SensorDHT11_T(5, 2);
  sensors[1] = airSensor;//air t
  sensors[2] = new SensorDHT11_Hum(airSensor, 3);// air hum
#endif
#ifdef BUILD_SHELF1
  sensors = new Sensor *[7];
  sensorCount = 7;
  sensors[0] = new SensorBH1750(0X23, 4, BH1750_CONTINUOUS_HIGH_RES_MODE);
  sensors[1] = new SensorBH1750(0X5C, 5, BH1750_CONTINUOUS_HIGH_RES_MODE);
  sensors[2] = new SensorBH1750(0X23, 6, BH1750_CONTINUOUS_HIGH_RES_MODE);
  sensors[3] = new SensorBH1750(0X5C, 7, BH1750_CONTINUOUS_HIGH_RES_MODE);
  sensors[4] = new SensorSEN0161(0, 8);//ph
  sensors[5] = new SensorDS18B20(A1, 9);//water t
  sensors[6] = new SensorDFR0300(2, sensors[1], 10);//ec
#endif
#ifdef BUILD_SHELF2
  sensors = new Sensor *[0];
  sensorCount = 0;
  /*sensors[0] = new SensorBH1750(0X23, 4, BH1750_CONTINUOUS_HIGH_RES_MODE);
  sensors[1] = new SensorBH1750(0X5C, 5, BH1750_CONTINUOUS_HIGH_RES_MODE);
  sensors[2] = new SensorBH1750(0X23, 6, BH1750_CONTINUOUS_HIGH_RES_MODE);
  sensors[3] = new SensorBH1750(0X5C, 7, BH1750_CONTINUOUS_HIGH_RES_MODE);
  sensors[4] = new SensorSEN0161(0, 8);//ph
  sensors[5] = new SensorDS18B20(A1, 9);//water t
  sensors[6] = new SensorDFR0300(2, sensors[1], 10);//ec*/
#endif

  for (int i = 0; i < sensorCount; i++)
  {
    sensors[i]->init();
  }

#if defined(DEBUG_ETH) || !defined(DEBUG)
  Serial.println("eth enabled");
  if (!Ethernet.begin(mac))
  {
    Serial.println("Ethernet not started");
  }
#endif

  Serial.println("Setup done");
  
  wdt_enable(WDTO_8S);
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
}

void loop()
{
  digitalWrite(13, HIGH);
  String data;

  for (int i = 0; i < sensorCount; i++)
  {
    wdt_reset();
    Sensor *s = sensors[i];

    printSensorType(s->getType());

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

  digitalWrite(13, LOW);
  //send data to server
#if defined(DEBUG_ETH) || !defined(DEBUG)
  sendDataToServer(&data);
#endif


  delayWDT(period);
  if (millis() > resetTime) {
    while(1);
  }
}

void delayWDT(unsigned long delayTime) {
  unsigned long time = millis();
  unsigned long current = millis();
  while (current-time<delayTime) {
    current = millis();
    wdt_reset();
  }
}

void addSensorInfoToData(String *data, uint8_t sensorId, uint8_t type, String model, uint8_t errorCode, float value)
{
  *data = *data + String(sensorId) + ',' + String(type) + ',' + String(model) + ',' + String(errorCode) + ',' + String(value) + "\r\n";
}

void sendDataToServer(String *data)
{
  if (Ethernet.gatewayIP() == NULL)
  {
    Serial.println("no gateway");
    Ethernet.begin(mac);
  }
  else
  {
    // start the Ethernet connection:
    Ethernet.maintain();
    Serial.println("ethernet ok");
  }

  //renew connection
  client.stop();
  if (client.connect(server, port))
  {
    client.println("POST /sensorInput HTTP/1.1");
    client.println("Host: hydroponics.vo-it.ru");
    client.println("Content-Type: text/csv");
    client.print("Content-Length: ");
    *data = "test,test,test";
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
}

