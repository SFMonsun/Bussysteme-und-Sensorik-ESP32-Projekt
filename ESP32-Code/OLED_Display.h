#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <Adafruit_SSD1306.h>
#include "Sensors.h"
#include "Error_Detect.h"

void initDisplay(Adafruit_SSD1306 &display);
void updateDisplay(Adafruit_SSD1306 &display, const SensorData &data, const ErrorStats &errors);

#endif // OLED_DISPLAY_H
