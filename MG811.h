#include "Sensor.h"

/************************Hardware Related Macros*********|***************************/
#define DC_GAIN 8.5 //define the DC gain of amplifier

/***********************Software Related Macros************************************/
#define READ_SAMPLE_TIMES 10    //define how many samples you are going to take in normal operation
#define READ_SAMPLE_INTERVAL 50 //define the time interval(in milisecond) between each samples in
//normal operation

/**********************Application Related Macros**********************************/
//These values differ from sensor to sensor. User should derermine this value.
#define ZERO_POINT_X 2.602       //lg400=2.602, the start point_on X_axis of the curve
#define ZERO_POINT_VOLTAGE 0.324 //define the output of the sensor in volts when the concentration of CO2 is 400PPM
#define MAX_POINT_VOLTAGE 0.265  //define the output of the sensor in volts when the concentration of CO2 is 10,000PPM
#define REACTION_VOLTGAE 0.059   //define the voltage drop of the sensor when move the sensor from air into 1000ppm CO2

/*****************************Globals***********************************************/
float CO2Curve[3] = {ZERO_POINT_X, ZERO_POINT_VOLTAGE, (REACTION_VOLTGAE / (2.602 - 4))};
//Two points are taken from the curve.With these two points, a line is formed which is
//"approximately equivalent" to the original curve. You could use other methods to get more accurate slope

//CO2 Curve format:{ x, y, slope};point1: (lg400=2.602, 0.324), point2: (lg10000=4, 0.265)
//slope = (y1-y2)(i.e.reaction voltage)/ x1-x2 = (0.324-0.265)/(log400 - log10000)

//CO2 sensor
class SensorMG811 : public Sensor
{
public:
  SensorMG811(uint8_t analogPin, uint8_t digitalPin, uint8_t sensorId)
  {
    _analogPin = analogPin;
    _digitalPin = digitalPin;
    _sId = sensorId;
    _type = S_TYPE_CO2;
    _model = "MG811";
  }

  void init()
  {
    pinMode(_digitalPin, INPUT);
    digitalWrite(_digitalPin, HIGH);
  }

  uint8_t read()
  {
    int i;
    float v = 0;
    for (i = 0; i < READ_SAMPLE_TIMES; i++)
    {
      v += analogRead(_analogPin);
      delay(READ_SAMPLE_INTERVAL);
    }
    v = (v / READ_SAMPLE_TIMES) * 5 / (1024);
    v = v / DC_GAIN;
    if (v > ZERO_POINT_VOLTAGE || v < MAX_POINT_VOLTAGE)
    {
      _value = 350;
      return S_OUT_OF_RANGE;
    }
    else
    {
      _value = pow(10, (v - CO2Curve[1]) / CO2Curve[2] + CO2Curve[0]);
      return S_OK;
    }
  }

  float getData()
  {
    return _value;
  }

private:
  float _value;
  uint8_t _analogPin;
  uint8_t _digitalPin;
};

