#include "Sensor.h"

//Air temperature
class SensorDHT11_T : public Sensor
{
  public:
    SensorDHT11_T(uint8_t digitalPin, uint8_t sensorId)
    {
      _digitalPin = digitalPin;
      _sId = sensorId;
      _type = S_TYPE_T_AIR;
      _model = "DHT11";
    }

    void init()
    {
    }

    uint8_t read()
    {
      uint8_t bits[5];
      uint8_t cnt = 7;
      uint8_t idx = 0;

      // очистка буффера
      for (int i = 0; i < 5; i++)
        bits[i] = 0;

      _humidity = 0;
      _temperature = 0;

      // согласование с датчиком
      pinMode(_digitalPin, OUTPUT);
      digitalWrite(_digitalPin, LOW);
      delay(18);
      digitalWrite(_digitalPin, HIGH);
      delayMicroseconds(40);
      pinMode(_digitalPin, INPUT);

      readResult = S_OK;
      do
      {
        // проверка отвечает ли датчик
        unsigned int loopCnt = 10000;
        while (digitalRead(_digitalPin) == LOW)
          if (loopCnt-- == 0)
          {
            readResult = S_ERROR_TIMEOUT;
            break;
          }

        if (readResult != S_OK) break;

        loopCnt = 10000;
        while (digitalRead(_digitalPin) == HIGH)
          if (loopCnt-- == 0)
          {
            readResult = S_ERROR_TIMEOUT;
            break;
          }
        if (readResult != S_OK) break;

        // Считываем 40 бит
        for (int i = 0; i < 40; i++)
        {
          loopCnt = 10000;
          while (digitalRead(_digitalPin) == LOW)
            if (loopCnt-- == 0)
            {
              readResult = S_ERROR_TIMEOUT;
              break;
            }

          if (readResult != S_OK) break;

          unsigned long t = micros();
          loopCnt = 10000;
          while (digitalRead(_digitalPin) == HIGH)
            if (loopCnt-- == 0)
            {
              readResult = S_ERROR_TIMEOUT;
              break;
            }

          if (readResult != S_OK) break;

          if ((micros() - t) > 40)
            bits[idx] |= (1 << cnt);
          // следующий байт?
          if (cnt == 0)
          {
            cnt = 7;
            idx++;
          }
          else
          {
            cnt--;
          }
        }

        // запись данных
        _humidity = bits[0];
        _temperature = bits[2];
        // проверка контрольной суммы
        uint8_t sum = bits[0] + bits[2];

        if (bits[4] != sum)
        {
          readResult = S_ERROR_CHECKSUM;
        }
      } while (false);
      return readResult;
    }

    float getData()
    {
      return _temperature;
    }

  public:
    float _temperature;
    float _humidity;
    uint8_t _digitalPin;
    uint8_t readResult;
};

//Air humidity
class SensorDHT11_Hum : public Sensor
{
  public:
    SensorDHT11_Hum(SensorDHT11_T *source, uint8_t sensorId)
    {
      this->source = source;
      _sId = sensorId;
      _type = S_TYPE_HUMIDITY;
      _model = "DHT11";
    }

    void init()
    {
    }

    uint8_t read()
    {
      return source->readResult;
    }

    float getData()
    {
      return source->_humidity;
    }

  private:
    SensorDHT11_T *source;
};
