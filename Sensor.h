#ifndef SENSOR_H_
#define SENSOR_H_

#include <Arduino.h>

#define S_OK             0
#define S_OUT_OF_RANGE   1
#define S_ERROR_CHECKSUM 2
#define S_ERROR_TIMEOUT  3

class Sensor 
{
public: 
    virtual void init() = 0;
    virtual uint8_t read() = 0;
    virtual float getData() = 0;
    uint8_t getSId() { return _sId; }
    uint8_t getPId() { return _pId; }
protected:
    uint8_t _sId;
    uint8_t _pId;
};

#endif
