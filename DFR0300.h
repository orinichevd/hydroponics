#include "Sensor.h"
#include "OneWire.h"

#define StartConvert 0
#define ReadTemperature 1

//EC sensor return conductivity in ms/cm
class SensorDFR0300 : public Sensor
{
  public:
    SensorDFR0300(uint8_t analogPin, Sensor *temperatureSensor, uint8_t sensorId, uint8_t sensorPId)
    {
      _analogPin = analogPin;
      _sId = sensorId;
      _pId = sensorPId;
      this->temperatureSensor = temperatureSensor;
    }

    void init()
    {
    }

    uint8_t read()
    {
      uint8_t buf[numReadings];
      uint8_t average;
      float voltage;

      float _temperature = (temperatureSensor != NULL) ? temperatureSensor->getData() : 25.0;

      for (int i = 0; i < numReadings; i++)
      {
        // the readings from the analog input
        buf[i] = analogRead(_analogPin);
        average += buf[i];
        delay(25);
      }
      average = average / numReadings;
      voltage = average * 5000.0 / 1024;
      float tempCoeff = 1.0 + 0.0185 * (_temperature - 25.0);
      float coeffVoltage = voltage / tempCoeff;
      if (coeffVoltage < 150 || coeffVoltage > 3300)
      {
        return S_OUT_OF_RANGE;
      }
      else if (coeffVoltage <= 448)
      {
        _ecValue = 6.84 * coeffVoltage - 64.32;
      }
      else if (coeffVoltage <= 1457)
      {
        _ecValue = 6.98 * coeffVoltage - 127;
      }
      else
      {
        _ecValue = 5.3 * coeffVoltage + 2278;
      }
      _ecValue /= 1000;
      return S_OK;
    }

    float getData()
    {
      return _ecValue;
    }

  private:
    uint8_t _analogPin;

    float _ecValue;

    Sensor *temperatureSensor;

    const uint8_t numReadings = 25;
};

//returns the temperature from one DS18B20 in DEG Celsius
class SensorDS18B20 : public Sensor
{
  public:
    SensorDS18B20(uint8_t oneWirePin, uint8_t sensorId, uint8_t sensorPId)
    {
      _oneWirePin = oneWirePin;
      _sId = sensorId;
      _pId = sensorPId;
      ds = new OneWire(oneWirePin);
    }

    void init()
    {
      convert();
    }

    uint8_t read()
    {
      byte data[12];
      byte present = ds->reset();
      ds->select(addr);
      if (addr[0] != 0x10 && addr[0] != 0x28)
      {
        Serial.print("Device is not recognized!");
        return S_NOT_RECOGNIZED;
      }
      ds->write(0xBE); // Read Scratchpad
      for (int i = 0; i < 9; i++)
      { // we need 9 bytes
        data[i] = ds->read();
      }
      ds->reset_search();
      byte MSB = data[1];
      byte LSB = data[0];
      float tempRead = ((MSB << 8) | LSB); //using two's compliment
      _temperature = tempRead / 16;
      convert();
      return S_OK;
    }
    
    float getData() 
    {
      return _temperature;
    }

    void convert()
    {
      if (!ds->search(addr))
      {
        Serial.println("no more sensors on chain, reset search!");
        ds->reset_search();
        return;
      }
      if (OneWire::crc8(addr, 7) != addr[7])
      {
        Serial.println("CRC is not valid!");
        return;
      }
      if (addr[0] != 0x10 && addr[0] != 0x28)
      {
        Serial.print("Device is not recognized!");
        return;
      }
      ds->reset();
      ds->select(addr);
      ds->write(0x44, 1); // start conversion, with parasite power on at the end
    }

  private:
    uint8_t _oneWirePin;
    float _temperature;
    byte addr[8];
    OneWire* ds;
};
