#include "OLED_Display.h"

void initDisplay(Adafruit_SSD1306 &display) {
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 init failed"));
    for (;;) delay(10);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.display();
}

void updateDisplay(Adafruit_SSD1306 &display, const SensorData &data, const ErrorStats &errors) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.printf("Temp: %.2f C\n", data.temperature);
  display.printf("Press: %.2f hPa\n", data.pressure);
  display.printf("Light: %.2f lx\n\n", data.light);
  display.printf("I2C OK:%lu\n", errors.ok);
  display.printf("NA:%lu ND:%lu\n", errors.nack_addr, errors.nack_data);
  display.printf("Oth:%lu T:%lu\n", errors.other, errors.total);
  display.display();
}
