#ifndef SENSOR_H_
#define SENSOR_H_

#include <Arduino.h>

#define S_OK             0
#define S_OUT_OF_RANGE   1
#define S_ERROR_CHECKSUM 2
#define S_ERROR_TIMEOUT  3
#define S_NOT_RECOGNIZED 4

#define S_TYPE_T_AIR 1
#define S_TYPE_T_WATER 2
#define S_TYPE_EC 3
#define S_TYPE_PH 4
#define S_TYPE_CO2 5
#define S_TYPE_LIGHT 6
#define S_TYPE_HUMIDITY 7

class Sensor 
{
public: 
    virtual void init() = 0;
    virtual uint8_t read() = 0;
    virtual float getData() = 0;
    uint8_t getSId() { return _sId; }
    uint8_t getType() { return _type; }
    String getModel() { return _model; }
protected:
    uint8_t _sId;
    uint8_t _type;
    String _model;
};

#endif
