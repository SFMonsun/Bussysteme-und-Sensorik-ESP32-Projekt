#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BMP3XX.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>
#include <Adafruit_CCS811.h>
#include <Adafruit_SHTC3.h>

// ==== OLED settings ====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_SDA 21
#define OLED_SCL 22
#define LED1 12
#define LED2 13
#define LED3 14
bool ledState = false;
unsigned long lastBlink = 0;

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

// ==== CCS811 ====
Adafruit_CCS811 ccs;

// ==== SHTC3 ====
Adafruit_SHTC3 shtc3 = Adafruit_SHTC3();

// ==== Error counter ====
uint32_t i2cErrorCount = 0;

// ==== Setup ====
void setup() {
  Serial.begin(115200);
  delay(1000);

  // LED pins
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
  digitalWrite(LED3, LOW);

  // Start I2C buses
  SensorWire.begin(SENSOR_SDA, SENSOR_SCL);
  OLEDWire.begin(OLED_SDA, OLED_SCL);

  // ==== OLED init ====
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
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
    Serial.println("BMP390 not found!");
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

  // ==== CCS811 ====
  if (!ccs.begin(CCS811_ADDRESS, &SensorWire)) {
    Serial.println("CCS811 not found!");
    display.println("CCS811 fail!");
    display.display();
  } else {
    Serial.println("CCS811 OK");
    // The CCS811 needs some warm-up time
    while (!ccs.available()) {
      delay(100);
      Serial.println("CCS811 warming up...");
    }
  }

  // ==== SHTC3 ====
  if (!shtc3.begin(&SensorWire)) {
    Serial.println("SHTC3 not found!");
    display.println("SHTC3 fail!");
    display.display();
  } else {
    Serial.println("SHTC3 OK");
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
  bool okTSL = event.light != 0 || event.light == 0;
  if (!okTSL) {
    i2cErrorCount++;
    Serial.println("TSL2561 read failed");
  }

  // ==== CCS811 ====
  bool okCCS = false;
  float co2 = NAN;
  float tvoc = NAN;
  if (ccs.available()) {
    if (!ccs.readData()) {
      co2 = ccs.geteCO2();
      tvoc = ccs.getTVOC();
      okCCS = true;
    } else {
      i2cErrorCount++;
      Serial.println("CCS811 read failed");
    }
  }
    if (!isnan(co2) && co2 >= 1000) {
    unsigned long now = millis();
    if (now - lastBlink >= 1000) { // toggle every 1s
      ledState = !ledState;
      digitalWrite(LED1, ledState);
      digitalWrite(LED2, ledState);
      digitalWrite(LED3, ledState);
      lastBlink = now;
    }
  } else {
    // CO2 normal: ensure LEDs off
    ledState = false;
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);
  }

  // ==== SHTC3 ====
  sensors_event_t humidity, temp;
  bool okSHTC = false;
  if (shtc3.getEvent(&humidity, &temp)) {
    okSHTC = true;
  } else {
    i2cErrorCount++;
    Serial.println("SHTC3 read failed");
  }

  // Reset counter at 10,000
  if (i2cErrorCount >= 10000) {
    i2cErrorCount = 0;
    Serial.println("Error counter reset after 10,000");
  }

  float temp_bmp = bmp.temperature;
  float pressure = bmp.pressure / 100.0; // hPa
  float lux = event.light ? event.light : 0;
  float hum = okSHTC ? humidity.relative_humidity : NAN;

  // ==== Serial output ====
  Serial.printf("LM75B: %.2f C | BMP390: %.2f C | %.2f hPa | Lux: %.2f | Hum: %.1f%% | CO2: %.0f ppm | TVOC: %.0f ppb | Err: %lu\n",
                temp_lm75, temp_bmp, pressure, lux, hum, co2, tvoc, i2cErrorCount);

  // ==== OLED display ====
  display.clearDisplay();
  display.setCursor(0, 0);
  //display.printf("LM75B: %.1fC  BMP: %.1fC\n", temp_lm75, temp_bmp);
  display.printf("LM75B: %.1fC\n", temp_lm75);
  display.printf("P: %.1f hPa  L:%.0f lx\n", pressure, lux);
  display.printf("L: %.0f lx\n", lux);
  display.printf("Hum: %.1f\n", hum);
  display.printf("CO2: %.0fppm\n", co2);
  display.printf("TVOC: %.0fppb Err:%lu\n", tvoc, i2cErrorCount);
  display.printf("Err:%lu\n", i2cErrorCount);
  display.display();

  delay(1000);
}