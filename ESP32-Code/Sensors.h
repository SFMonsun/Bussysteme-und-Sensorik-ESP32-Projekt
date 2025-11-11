#ifndef SENSORS_H
#define SENSORS_H

#include <Wire.h>

struct SensorData {
  float temperature;
  float pressure;
  float light;
};

void initSensors(TwoWire &bus);
SensorData readAllSensors(TwoWire &bus);

#endif // SENSORS_H
