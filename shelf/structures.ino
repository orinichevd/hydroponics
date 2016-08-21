struct ConfigFile {
  unsigned int arduinoId;//arduino id
  byte mac[6];//arduino mac
  char url[20];//url of the server
  unsigned int port;//server port
  byte sensorCount;//number of sensors on board
  unsigned int sensorType[20];//types of sensors
  byte pinNumber[20];//number of sensor pin
  unsigned int sensorId[20];//global system id of sensor
};

struct EthernetData {
};
