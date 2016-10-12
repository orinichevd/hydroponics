#include "Sensor.h"

#include <Wire.h>


// I2C commands
#define RH_READ 0xE5
#define TEMP_READ 0xE3
#define POST_RH_TEMP_READ 0xE0
#define RESET 0xFE
#define USER1_READ 0xE7
#define USER1_WRITE 0xE6

// compound commands
byte SERIAL1_READ[] = {0xFA, 0x0F};
byte SERIAL2_READ[] = {0xFC, 0xC9};

//light sensor lux
class SensorSI7021_H : public Sensor
{
  public:

    float _humidity;
    float _temperature;

    SensorSI7021_H(uint8_t address, uint8_t sensorId)
    {
      _address = address;
      _sId = sensorId;
      _type = S_TYPE_HUMIDITY;
      _model = "SI7021";
    }

    void init()
    {
      Wire.beginTransmission(_address);
      if (Wire.endTransmission() == 0)
      {
      }
    }

    uint8_t read()
    {
      uint8_t result = S_OK;

      byte humbytes[2];
      _command(RH_READ, humbytes);
      long humraw = (long)humbytes[0] << 8 | humbytes[1];
      _humidity = ((125 * humraw) / 65536) - 6;

      byte tempbytes[2];
      _command(POST_RH_TEMP_READ, tempbytes);
      long tempraw = (long)tempbytes[0] << 8 | tempbytes[1];
      _temperature = ((175.72 * tempraw) / 65536) - 46.85;

      return result;
    }

    float getData()
    {
      return _humidity;
    }

  private:

    void _writeReg(byte * reg, int reglen) {
      Wire.beginTransmission(_address);
      for (int i = 0; i < reglen; i++) {
        reg += i;
        Wire.write(*reg);
      }
      Wire.endTransmission();
    }

    void _readReg(byte * reg, uint8_t reglen) {
      Wire.requestFrom(_address, reglen);
      while (Wire.available() < reglen) {
      }
      for (int i = 0; i < reglen; i++) {
        reg[i] = Wire.read();
      }
    }

    void _command(byte cmd, byte * buf ) {
      _writeReg(&cmd, sizeof cmd);
      _readReg(buf, 2);
    }

    uint8_t _address;
};

class SensorSI7021_T : public Sensor
{
  public:
    SensorSI7021_T(SensorSI7021_H *source, uint8_t sensorId)
    {
      this->source = source;
      _sId = sensorId;
      _type = S_TYPE_T_AIR;
      _model = "SI7021";
    }

    void init()
    {
    }

    uint8_t read()
    {
      return S_OK;
    }

    float getData()
    {
      return source->_temperature;
    }

  private:
    SensorSI7021_H *source;
};
