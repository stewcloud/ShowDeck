#include "elecrow_5.h"

#ifdef SHOWDECK_ELECROW_5

#include <Arduino.h>
#include <Wire.h>

namespace {
constexpr uint8_t pca9557Address = 0x18;
constexpr uint8_t pca9557Output = 0x01;
constexpr uint8_t pca9557Config = 0x03;
constexpr uint8_t gt911Addresses[] = {0x5D, 0x14};
constexpr uint16_t gt911PointInfo = 0x814E;
constexpr uint16_t gt911FirstPoint = 0x814F;
constexpr uint16_t gt911ProductId = 0x8140;
uint8_t gt911Address = 0;
bool lastTouchDown = false;
uint32_t lastTouchReport = 0;

uint8_t expanderRead(uint8_t reg) {
  Wire.beginTransmission(pca9557Address);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return 0xFF;
  Wire.requestFrom(pca9557Address, static_cast<uint8_t>(1));
  return Wire.available() ? Wire.read() : 0xFF;
}

void expanderWrite(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(pca9557Address);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

bool gt911ReadAt(uint8_t address, uint16_t reg, uint8_t *data, size_t size) {
  Wire.beginTransmission(address);
  Wire.write(highByte(reg));
  Wire.write(lowByte(reg));
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(address, size) != size) return false;
  for (size_t i = 0; i < size; ++i) data[i] = Wire.read();
  return true;
}

bool gt911Read(uint16_t reg, uint8_t *data, size_t size) {
  return gt911Address && gt911ReadAt(gt911Address, reg, data, size);
}

void clearTouchStatus() {
  if (!gt911Address) return;
  Wire.beginTransmission(gt911Address);
  Wire.write(highByte(gt911PointInfo));
  Wire.write(lowByte(gt911PointInfo));
  Wire.write(0);
  Wire.endTransmission();
}

void resetTouchHardware() {
  // DIS07050H V3.0 routes GT911 reset and interrupt through a PCA9557.
  expanderWrite(pca9557Config, 0xFC);
  expanderWrite(pca9557Output, expanderRead(pca9557Output) & 0xFC);
  delay(20);
  expanderWrite(pca9557Output, expanderRead(pca9557Output) | 0x01);
  delay(100);
  expanderWrite(pca9557Config, expanderRead(pca9557Config) | 0x02);
}
}  // namespace

Elecrow5Display::Elecrow5Display() {
  auto bus = bus_.config();
  bus.panel = &panel_;
  const int8_t pins[16] = {8, 3, 46, 9, 1, 5, 6, 7, 15, 16, 4, 45, 48, 47, 21, 14};
  memcpy(bus.pin_data, pins, sizeof(pins));
  bus.pin_henable = 40;
  bus.pin_vsync = 41;
  bus.pin_hsync = 39;
  bus.pin_pclk = 0;
  bus.freq_write = 12000000;
  bus.hsync_polarity = 0;
  bus.hsync_front_porch = 8;
  bus.hsync_pulse_width = 4;
  bus.hsync_back_porch = 43;
  bus.vsync_polarity = 0;
  bus.vsync_front_porch = 8;
  bus.vsync_pulse_width = 4;
  bus.vsync_back_porch = 12;
  bus.pclk_active_neg = 1;
  bus.de_idle_high = 0;
  bus.pclk_idle_high = 0;
  bus_.config(bus);

  auto panel = panel_.config();
  panel.memory_width = 800;
  panel.memory_height = 480;
  panel.panel_width = 800;
  panel.panel_height = 480;
  panel.offset_x = 0;
  panel.offset_y = 0;
  panel_.config(panel);
  panel_.setBus(&bus_);
  setPanel(&panel_);
}

void initElecrowTouch() {
  Wire.begin(19, 20);
  Wire.setClock(400000);
  resetTouchHardware();
  uint8_t productId[4];
  for (uint8_t address : gt911Addresses) {
    if (gt911ReadAt(address, gt911ProductId, productId, sizeof(productId))) {
      gt911Address = address;
      Serial.printf("GT911 found at 0x%02X, ID %.4s\n", address, productId);
      return;
    }
  }
  Serial.println("GT911 touch controller not found");
}

bool readElecrowTouch(int16_t &x, int16_t &y) {
  uint8_t status;
  if (!gt911Read(gt911PointInfo, &status, 1) || !(status & 0x80)) {
    if (lastTouchDown && millis() - lastTouchReport < 40) return true;
    lastTouchDown = false;
    return false;
  }

  const uint8_t points = status & 0x0F;
  uint8_t point[7];
  const bool valid = points > 0 && points <= 5 &&
                     gt911Read(gt911FirstPoint, point, sizeof(point));
  clearTouchStatus();
  if (!valid) {
    lastTouchDown = false;
    return false;
  }

  x = constrain(point[1] | (point[2] << 8), 0, 799);
  y = constrain(point[3] | (point[4] << 8), 0, 479);
  lastTouchReport = millis();
  lastTouchDown = true;
  return true;
}

#endif
