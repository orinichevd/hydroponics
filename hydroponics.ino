#include <SPI.h>
#include <Ethernet2.h>

#include "MG811.h"
#include "DHT11.h"
#include "SEN0161.h"
#include "DFR0300.h"
#include "Sensor.h"


/************************Server configuration*********|***************************/
const char server[] = "https://hydroponics.eu-gb.mybluemix.net";
byte mac[] = {0x90, 0xA2, 0xDA, 0x10, 0x84, 0xDE};
const int port = 6001;

EthernetClient client;

Sensor** sensors = new Sensor*[3];
const unsigned int sensorCount = 3;

void setup()
{
  Serial.begin(9600);
  while (!Serial) {
    ;
  }
  delay(1000);
  Serial.println("Serial started");
  sensors[0] = new SensorSEN0161(0, 1, 4);//ph
  sensors[1] = new SensorDS18B20(1, 2, 6);//water t
  sensors[2] = new SensorDFR0300(2, sensors[1], 3, 7);//ec
  for (int i = 0; i < sensorCount; i++) {
    sensors[i]->init();
  }
  Ethernet.begin(mac);
  Serial.println("Setup done");
}

void loop()
{
  String data;

  for (int i = 0; i < sensorCount; i++)
  {
    Sensor* s = sensors[i];
    switch (s->getPId()) {
      case 1: Serial.print("CO2: "); break;
      case 2: Serial.print("T: "); break;
      case 3: Serial.print("Hum: "); break;
      case 4: Serial.print("Ph: "); break;
      case 5: Serial.print("Ec: "); break;
      case 6: Serial.print("Water T: "); break;
      case 7: Serial.print("Light: "); break;
    }
    uint8_t errorCode = s->read();
    switch (errorCode)
    {
      case S_OK :
        {
          float sensorData = s->getData();
          addSensorInfoToData(&data, s->getSId(), s->getPId(), errorCode, sensorData);
          Serial.println(sensorData);
        }
        break;
      case S_OUT_OF_RANGE:
        addSensorInfoToData(&data, s->getSId(), s->getPId(), errorCode, 0);
        Serial.println("out of range");
        break;
      case S_ERROR_CHECKSUM:
        addSensorInfoToData(&data, s->getSId(), s->getPId(), errorCode, 0);
        Serial.println("checksum error");
        break;
      case S_ERROR_TIMEOUT:
        addSensorInfoToData(&data, s->getSId(), s->getPId(), errorCode, 0);
        Serial.println("timeout error");
        break;
    }
  }

  //send data to server
  //sendDataToServer(&data);


  delay(5000);
}

void addSensorInfoToData( String* data
                          , uint8_t sensorId
                          , uint8_t sensorPId
                          , uint8_t errorCode
                          , float value) {
  *data = *data + String(sensorId) + ',' + String(sensorPId) + ',' + String(errorCode) + ',' + String(value) + "\r\n";
}

void sendDataToServer(String* data) {
  // start the Ethernet connection:
  Ethernet.maintain();
  //renew connection
  client.stop();
  if (client.connect(server, port)) {
    client.println("POST /hydro HTTP/1.1");
    client.println("Host: 192.168.0.21:6001");
    client.println("Content-Type: text/csv");
    client.print("Content-Length: ");
    client.println((*data).length());
    client.println();
    client.println((*data));
  }
  client.stop();
}


