#include "Error_Detect.h"

static ErrorStats stats = {0, 0, 0, 0, 0};

ErrorStats checkI2CErrors(TwoWire &bus, uint8_t addr) {
  bus.beginTransmission(addr);
  bus.write(0x00);
  uint8_t res = bus.endTransmission();

  stats.total++;
  switch (res) {
    case 0: stats.ok++; break;
    case 2: stats.nack_addr++; break;
    case 3: stats.nack_data++; break;
    default: stats.other++; break;
  }

  if (stats.total >= 10000) {
    stats.ok = stats.nack_addr = stats.nack_data = stats.other = stats.total = 0;
  }

  return stats;
}
