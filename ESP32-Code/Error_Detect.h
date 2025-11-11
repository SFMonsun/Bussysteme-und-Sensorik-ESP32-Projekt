#ifndef ERROR_DETECT_H
#define ERROR_DETECT_H

#include <Wire.h>

struct ErrorStats {
  uint32_t ok;
  uint32_t nack_addr;
  uint32_t nack_data;
  uint32_t other;
  uint32_t total;
};

ErrorStats checkI2CErrors(TwoWire &bus, uint8_t addr);

#endif // ERROR_DETECT_H
