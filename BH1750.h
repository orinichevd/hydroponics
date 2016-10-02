#include "Sensor.h"
#include <Wire.h>
#include <I2cMaster.h>

#define BH1750_I2CADDR 0x23

// No active state
#define BH1750_POWER_DOWN 0x00

// Wating for measurment command
#define BH1750_POWER_ON 0x01

// Reset data register value - not accepted in POWER_DOWN mode
#define BH1750_RESET 0x07

// Start measurement at 1lx resolution. Measurement time is approx 120ms.
#define BH1750_CONTINUOUS_HIGH_RES_MODE 0x10

// Start measurement at 0.5lx resolution. Measurement time is approx 120ms.
#define BH1750_CONTINUOUS_HIGH_RES_MODE_2 0x11

// Start measurement at 4lx resolution. Measurement time is approx 16ms.
#define BH1750_CONTINUOUS_LOW_RES_MODE 0x13

// Start measurement at 1lx resolution. Measurement time is approx 120ms.
// Device is automatically set to Power Down after measurement.
#define BH1750_ONE_TIME_HIGH_RES_MODE 0x20

// Start measurement at 0.5lx resolution. Measurement time is approx 120ms.
// Device is automatically set to Power Down after measurement.
#define BH1750_ONE_TIME_HIGH_RES_MODE_2 0x21

// Start measurement at 1lx resolution. Measurement time is approx 120ms.
// Device is automatically set to Power Down after measurement.
#define BH1750_ONE_TIME_LOW_RES_MODE 0x23

#define BH1750_ADDR_PIN_LOW 0XFF

//light sensor lux
class SensorBH1750 : public Sensor
{
public:
  SensorBH1750(uint8_t address, uint8_t sensorId, uint8_t mode = BH1750_CONTINUOUS_HIGH_RES_MODE, uint8_t _digitalPin = BH1750_ADDR_PIN_LOW)
  {
    _digitalPin = _digitalPin;
    _address = address;
    _sId = sensorId;
    _type = S_TYPE_LIGHT;
    _model = "BH1750";
    _mode = mode;
  }

  void init()
  {
    Wire.begin();
    Wire.beginTransmission(_address);
    Wire.write(_mode);
    Wire.endTransmission();
    if (_digitalPin != BH1750_ADDR_PIN_LOW)
    {
      pinMode(_digitalPin, OUTPUT);
      digitalWrite(_digitalPin, HIGHT);
    }
    ���� ����
  }

  uint8_t read()
  {
    uint8_t result = S_OK;
    uint16_t level;
    Wire.beginTransmission(_address);
    Wire.requestFrom(_address, 2);

    byte buff[2];
    int i = 0;
    while (Wire.available())
    {
      buff[i] = Wire.read();
      i++;
    }
    Wire.endTransmission();
    Serial.print(i);

    if (i == 2)
    {
      _value = ((buff[0] << 8) | buff[1]) / 1.2;
    }
    else
    {
      _value = 0;
      result = S_OUT_OF_RANGE;
    }
    return result;
  }

  float getData()
  {
    return _value;
  }

private:
  uint8_t _digitalPin;
  float _value;
  uint8_t _address;
  uint8_t _mode;
};

//light sensor lux
class SensorBH1750_Virtual : public Sensor
{
public:
  SensorBH1750(uint8_t sdaPin, uint8_t sclPin, uint8_t address, uint8_t sensorId, uint8_t mode = BH1750_CONTINUOUS_HIGH_RES_MODE, uint8_t _digitalPin = BH1750_ADDR_PIN_LOW)
  {
    _sdaPin = sdaPin;
    _sclPin = sclPin
        _digitalPin = _digitalPin;
    _address = address;
    _sId = sensorId;
    _type = S_TYPE_LIGHT;
    _model = "BH1750";
    _mode = mode;
  }

  void init()
  {
    Wire.begin();
    Wire.beginTransmission(_address);
    Wire.write(_mode);
    Wire.endTransmission();
    if (_digitalPin != BH1750_ADDR_PIN_LOW)
    {
      pinMode(_digitalPin, OUTPUT);
      digitalWrite(_digitalPin, HIGHT);
    }
  }

  uint8_t read()
  {
    uint8_t result = S_OK;
    uint16_t level;
    Wire.beginTransmission(_address);
    Wire.requestFrom(_address, 2);

    byte buff[2];
    int i = 0;
    while (Wire.available())
    {
      buff[i] = Wire.read();
      i++;
    }
    Wire.endTransmission();
    Serial.print(i);

    if (i == 2)
    {
      _value = ((buff[0] << 8) | buff[1]) / 1.2;
    }
    else
    {
      _value = 0;
      result = S_OUT_OF_RANGE;
    }

    return result;
  }

  float getData()
  {
    return _value;
  }

private:
  SoftI2cMaster *rtc;
  uint8_t _sdaPin, _sclPin;
  uint8_t _digitalPin;
  float _value;
  uint8_t _address;
  uint8_t _mode;
};
