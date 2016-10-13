#ifndef LOG_OR
#define LOG_OR

#include <Arduino.h>
#include <SD.h>

class Logger
{
  public:
    Logger(uint8_t csPin);
    void init();
    void logData(char* data);
    void logData(float data);

  private:
    uint8_t  _csPin;
};

#endif
