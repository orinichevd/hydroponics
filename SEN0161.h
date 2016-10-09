#include "Sensor.h"

//Ph sensor
class SensorSEN0161 : public Sensor
{
  public:
    SensorSEN0161(uint8_t analogPin, uint8_t sensorId)
    {
      _analogPin = analogPin;
      _sId = sensorId;
      _type = S_TYPE_PH;
      _model = "SEN0161";
    }

    void init()
    {
    }

    uint8_t read()
    {
      unsigned long int avgValue; //Store the average value of the sensor feedback
      float b;
      int buf[10], temp;

      for (int i = 0; i < 10; i++) //Get 10 sample value from the sensor for smooth the value
      {
        buf[i] = analogRead(_analogPin);
        delay(10);
      }
      for (int i = 0; i < 9; i++) //sort the analog from small to large
      {
        for (int j = i + 1; j < 10; j++)
        {
          if (buf[i] > buf[j])
          {
            temp = buf[i];
            buf[i] = buf[j];
            buf[j] = temp;
          }
        }
      }
      avgValue = 0;
      for (int i = 2; i < 8; i++) //take the average value of 6 center sample
        avgValue += buf[i];
      float phValue = (float)avgValue * 5.0 / 1024 / 6; //convert the analog into millivolt
      _phValue = 3.5 * phValue;                         //convert the millivolt into pH value
      return S_OK;
    }

    float getData()
    {
      return _phValue;
    }

  private:
    uint8_t _analogPin;
    float _phValue;
};

