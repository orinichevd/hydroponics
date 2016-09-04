#define SCL_PIN 2 
#define SCL_PORT PORTD 
#define SDA_PIN 0 
#define SDA_PORT PORTC
#include <SoftI2CMaster.h>
#include <SPI.h>
#include <Ethernet2.h>

char server[] = "https://hydroponics.eu-gb.mybluemix.net";
byte mac[] = {0x90, 0xA2, 0xDA, 0x10, 0x77, 0xC8};
int port = 6001;
IPAddress ip(192, 168, 0, 21);

EthernetClient client;

void setup() {
  //load data from EEPROM

  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  delay(1000);
  Ethernet.begin(mac);
}

void loop() {
  String data;
  // get data from sensors
  addSensorInfoToData(&data, 1, 1.0);
  addSensorInfoToData(&data, 2, 2.0);
  //send data to server
  sendDataToServer(&data);
  delay(5000);
}

void addSensorInfoToData(String* data, unsigned int sensorId, float value) {
  *data = *data + String(sensorId) + ',' + String(value) + "\r\n";
}

void sendDataToServer(String* data) {
  // start the Ethernet connection:
  Ethernet.maintain();
  //renew connection
  client.stop();
  if (client.connect(ip, port)) {
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
