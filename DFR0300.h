#include "Sensor.h"
#include "OneWire.h"

#define BUILD_SHELF2

//EC sensor return conductivity in ms/cm
class SensorDFR0300 : public Sensor
{
  public:
    SensorDFR0300(uint8_t analogPin, Sensor *temperatureSensor, uint8_t sensorId)
    {
      _analogPin = analogPin;
      _sId = sensorId;
      _type = S_TYPE_EC;
      _model = "DS18B20";
      this->temperatureSensor = temperatureSensor;
    }

    void init()
    {
    }

    uint8_t read()
    {
      uint8_t buf[25];
      float average = 0;
      float voltage;

      float _temperature = (temperatureSensor != NULL) ? temperatureSensor->getData() : 25.0;

      for (int i = 0; i < 25; i++)
      {
        // the readings from the analog input
        average += analogRead(_analogPin);
      }

      average = average / 25;
      _ecValue = average;
      /*voltage = average * 4.88;
      float tempCoeff = 1.0 + 0.0185 * (_temperature - 25.0);
      float coeffVoltage = voltage / tempCoeff;
      if (coeffVoltage < 150 || coeffVoltage > 3300)
      {
        return S_OUT_OF_RANGE;
      }
      else
      {
        #ifdef BUILD_SHELF1
        _ecValue = 0.008*coeffVoltage - 0.045;
        #endif
        #ifdef BUILD_SHELF2
        _ecValue = 0.008*coeffVoltage - 0.124;
        #endif
      }*/
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
};

//returns the temperature from one DS18B20 in DEG Celsius
class SensorDS18B20 : public Sensor
{
  public:
    SensorDS18B20(uint8_t oneWirePin, uint8_t sensorId)
    {
      _oneWirePin = oneWirePin;
      _sId = sensorId;
      _type = S_TYPE_T_WATER;
      _model = "DS18B20";
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

        ds->reset_search();
        return;
      }
      if (OneWire::crc8(addr, 7) != addr[7])
      {
        return;
      }
      if (addr[0] != 0x10 && addr[0] != 0x28)
      {
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

