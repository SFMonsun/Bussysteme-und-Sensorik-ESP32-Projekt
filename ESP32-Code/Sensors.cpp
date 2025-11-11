#include "Sensors.h"
#include <Adafruit_BMP3XX.h>
#include <Adafruit_TSL2561_U.h>

Adafruit_BMP3XX bmp;
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

// LM75B I2C address (usually 0x48)
#define LM75B_ADDR 0x48

void initSensors(TwoWire &bus) {
  // BMP390
  if (!bmp.begin_I2C(0x76, &bus)) {
    Serial.println("BMP390 not found!");
  }

  // TSL2561
  if (!tsl.begin(&bus)) {
    Serial.println("TSL2561 not found!");
  }
  tsl.enableAutoRange(true);
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);
}

// Reads LM75B temperature directly over I2C
float readLM75B(TwoWire &bus) {
  bus.beginTransmission(LM75B_ADDR);
  bus.write(0x00); // Temperature register
  if (bus.endTransmission(false) != 0) return NAN;

  if (bus.requestFrom(LM75B_ADDR, (uint8_t)2) != 2) return NAN;
  uint8_t msb = bus.read();
  uint8_t lsb = bus.read();

  // LM75B outputs temperature in 9-bit two’s complement (0.5°C resolution)
  int16_t raw = ((msb << 8) | lsb) >> 7;
  if (raw > 255) raw -= 512;
  return raw * 0.5;
}

SensorData readAllSensors(TwoWire &bus) {
  SensorData d{};

  // LM75B
  d.temperature = readLM75B(bus);

  // BMP390
  if (!bmp.performReading()) {
    d.pressure = NAN;
  } else {
    d.pressure = bmp.pressure / 100.0; // Pa → hPa
  }

  // TSL2561
  sensors_event_t event;
  tsl.getEvent(&event);
  d.light = event.light;

  return d;
}
