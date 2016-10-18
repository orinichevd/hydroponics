#include "log.h"
//#define LOG_SERIAL

Logger::Logger(uint8_t csPin)
{
  _csPin = csPin;
}

void Logger::init()
{
#ifdef LOG_SERIAL
  Serial.begin(9600);
  while (!Serial)
    ;
  delay(1000);
  Serial.println("Serial started");
#endif
#ifdef LOG_SD
  SD.begin(chipSelect);
#endif
}

void Logger::logData(char *data)
{
#ifdef LOG_SD
  File logFile = SD.open("log", FILE_WRITE);
  if (logFile)
  {
    logFile.println(String(millis()) + " " + data);
    logFile.close();
  }
#endif
#ifdef LOG_SERIAL
  Serial.println(data);
#endif
}

void Logger::logData(float data)
{
  int fractpart, intpart;
  intpart = trunc(data);
  fractpart = trunc((data - trunc(data)) * 100);
  char buf[200];
  buf[0] = '\0';
  sprintf(buf, "%d: %d.%d", millis(), intpart, fractpart);
#ifdef LOG_SD
  File logFile = SD.open("log", FILE_WRITE);
  if (logFile)
  {
    logFile.println(buf);
    logFile.close();
  }
#endif
#ifdef LOG_SERIAL
  Serial.println(buf);
#endif
}
