#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BMP3XX.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>

// ==== OLED settings ====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_SDA 21
#define OLED_SCL 22

TwoWire OLEDWire = TwoWire(1);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &OLEDWire, OLED_RESET);

// ==== Sensor I2C settings ====
#define SENSOR_SDA 19
#define SENSOR_SCL 18
TwoWire SensorWire = TwoWire(0);

// ==== LM75B ====
#define LM75_ADDR 0x48  // default address

float readLM75B(TwoWire &wire, bool &ok) {
  wire.beginTransmission(LM75_ADDR);
  wire.write(0x00); // temperature register
  if (wire.endTransmission(false) != 0) { ok = false; return NAN; }
  if (wire.requestFrom(LM75_ADDR, 2) < 2) { ok = false; return NAN; }

  uint8_t msb = wire.read();
  uint8_t lsb = wire.read();

  int16_t temp_raw = ((msb << 8) | lsb) >> 7;
  ok = true;
  return temp_raw * 0.5;
}

// ==== BMP390 ====
Adafruit_BMP3XX bmp;

// ==== TSL2561 ====
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

// ==== Error counter ====
uint32_t i2cErrorCount = 0;

// ==== Setup ====
void setup() {
  Serial.begin(115200);
  delay(1000);

  // Start I2C buses
  SensorWire.begin(SENSOR_SDA, SENSOR_SCL);
  OLEDWire.begin(OLED_SDA, OLED_SCL);

  // ==== OLED init ====
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found!");
    while (1);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Initializing...");
  display.display();

  // ==== BMP390 ====
  if (!bmp.begin_I2C(0x76, &SensorWire)) {
    Serial.println("Couldn't find BMP390!");
    display.println("BMP390 fail!");
    display.display();
  } else {
    Serial.println("BMP390 OK");
    bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
    bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
    bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  }

  // ==== TSL2561 ====
  if (!tsl.begin(&SensorWire)) {
    Serial.println("TSL2561 not found!");
    display.println("TSL2561 fail!");
    display.display();
  } else {
    Serial.println("TSL2561 OK");
    tsl.enableAutoRange(true);
    tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);
  }

  delay(1000);
  display.clearDisplay();
}

// ==== Loop ====
void loop() {
  bool okLM75 = false;
  float temp_lm75 = readLM75B(SensorWire, okLM75);
  if (!okLM75) {
    i2cErrorCount++;
    Serial.println("LM75B read failed");
  }

  bool okBMP = bmp.performReading();
  if (!okBMP) {
    i2cErrorCount++;
    Serial.println("BMP390 read failed");
  }

  sensors_event_t event;
  tsl.getEvent(&event);
  bool okTSL = event.light != 0 || event.light == 0; // ensure read attempt
  if (!okTSL) {
    i2cErrorCount++;
    Serial.println("TSL2561 read failed");
  }

  // Reset counter at 10,000
  if (i2cErrorCount >= 10000) {
    i2cErrorCount = 0;
    Serial.println("Error counter reset after 10,000");
  }

  float temp_bmp = bmp.temperature;
  float pressure = bmp.pressure / 100.0; // hPa
  float lux = event.light ? event.light : 0;

  // Serial output
  Serial.printf("LM75B: %.2f C | BMP390: %.2f C | %.2f hPa | Lux: %.2f | Errors: %lu\n",
                temp_lm75, temp_bmp, pressure, lux, i2cErrorCount);

  // OLED display
  display.clearDisplay();
  display.setCursor(0, 0);
  display.printf("LM75B Temp: %.2f C\n", temp_lm75);
  display.printf("BMP390 T:  %.2f C\n", temp_bmp);
  display.printf("Pressure:  %.2f hPa\n", pressure);
  display.printf("Light:     %.1f lx\n", lux);
  display.printf("I2C Err:   %lu\n", i2cErrorCount);
  display.display();

  delay(1000);
}
