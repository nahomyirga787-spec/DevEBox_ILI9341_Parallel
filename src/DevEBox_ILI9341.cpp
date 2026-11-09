#include "DevEBox_ILI9341.h"

DevEBox_ILI9341::DevEBox_ILI9341()
  : Adafruit_GFX(DEVBOX_TFT_WIDTH, DEVBOX_TFT_HEIGHT) {
  for (uint8_t i = 0; i < 16; ++i) {
    _dataPins[i] = DEVBOX_DEFAULT_DATA_PINS[i];
  }
}

void DevEBox_ILI9341::writeBus(uint16_t value) {
  for (uint8_t i = 0; i < 16; ++i) {
    digitalWrite(_dataPins[i], (value >> i) & 0x01);
  }
}

void DevEBox_ILI9341::pulseWR() {
  digitalWrite(DEVBOX_LCD_WR, LOW);
  digitalWrite(DEVBOX_LCD_WR, HIGH);
}

void DevEBox_ILI9341::writeCommand(uint8_t command) {
  digitalWrite(DEVBOX_LCD_RS, LOW);
  writeBus(command);
  pulseWR();
}

void DevEBox_ILI9341::writeData8(uint8_t data) {
  digitalWrite(DEVBOX_LCD_RS, HIGH);
  writeBus(data);
  pulseWR();
}

void DevEBox_ILI9341::writePixelBus(uint16_t color) {
  digitalWrite(DEVBOX_LCD_RS, HIGH);
  writeBus(color);
  pulseWR();
}

void DevEBox_ILI9341::resetDisplay() {
  digitalWrite(DEVBOX_LCD_RST, HIGH);
  delay(5);
  digitalWrite(DEVBOX_LCD_RST, LOW);
  delay(20);
  digitalWrite(DEVBOX_LCD_RST, HIGH);
  delay(150);
}

void DevEBox_ILI9341::setBacklight(bool enabled) {
#if DEVBOX_BACKLIGHT_ACTIVE_LOW
  digitalWrite(DEVBOX_LCD_BLK, enabled ? LOW : HIGH);
#else
  digitalWrite(DEVBOX_LCD_BLK, enabled ? HIGH : LOW);
#endif
}

void DevEBox_ILI9341::begin() {
  pinMode(DEVBOX_LCD_RS, OUTPUT);
  pinMode(DEVBOX_LCD_RD, OUTPUT);
  pinMode(DEVBOX_LCD_WR, OUTPUT);
  pinMode(DEVBOX_LCD_RST, OUTPUT);
  pinMode(DEVBOX_LCD_BLK, OUTPUT);

  for (uint8_t i = 0; i < 16; ++i) {
    pinMode(_dataPins[i], OUTPUT);
  }

  digitalWrite(DEVBOX_LCD_RD, HIGH);
  digitalWrite(DEVBOX_LCD_WR, HIGH);
  setBacklight(true);

  resetDisplay();
  initController();
}

void DevEBox_ILI9341::initController() {
  writeCommand(0x01);
  delay(150);

  writeCommand(0xCB);
  writeData8(0x39); writeData8(0x2C); writeData8(0x00);
  writeData8(0x34); writeData8(0x02);

  writeCommand(0xCF);
  writeData8(0x00); writeData8(0xC1); writeData8(0x30);

  writeCommand(0xE8);
  writeData8(0x85); writeData8(0x00); writeData8(0x78);

  writeCommand(0xEA);
  writeData8(0x00); writeData8(0x00);

  writeCommand(0xED);
  writeData8(0x64); writeData8(0x03);
  writeData8(0x12); writeData8(0x81);

  writeCommand(0xF7);
  writeData8(0x20);

  writeCommand(0xC0);
  writeData8(0x23);

  writeCommand(0xC1);
  writeData8(0x10);

  writeCommand(0xC5);
  writeData8(0x3E); writeData8(0x28);

  writeCommand(0xC7);
  writeData8(0x86);

  writeCommand(0x36);
  writeData8(0x28);

  writeCommand(0x3A);
  writeData8(0x55);

  writeCommand(0xB1);
  writeData8(0x00); writeData8(0x18);

  writeCommand(0xB6);
  writeData8(0x08); writeData8(0x82); writeData8(0x27);

  writeCommand(0xF2);
  writeData8(0x00);

  writeCommand(0x26);
  writeData8(0x01);

  writeCommand(0xE0);
  writeData8(0x0F); writeData8(0x31); writeData8(0x2B);
  writeData8(0x0C); writeData8(0x0E); writeData8(0x08);
  writeData8(0x4E); writeData8(0xF1); writeData8(0x37);
  writeData8(0x07); writeData8(0x10); writeData8(0x03);
  writeData8(0x0E); writeData8(0x09); writeData8(0x00);

  writeCommand(0xE1);
  writeData8(0x00); writeData8(0x0E); writeData8(0x14);
  writeData8(0x03); writeData8(0x11); writeData8(0x07);
  writeData8(0x31); writeData8(0xC1); writeData8(0x48);
  writeData8(0x08); writeData8(0x0F); writeData8(0x0C);
  writeData8(0x31); writeData8(0x36); writeData8(0x0F);

  writeCommand(0x11);
  delay(120);

  writeCommand(0x29);
  delay(20);
}

void DevEBox_ILI9341::setAddressWindow(
  uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {

  writeCommand(0x2A);
  writeData8(x1 >> 8);
  writeData8(x1 & 0xFF);
  writeData8(x2 >> 8);
  writeData8(x2 & 0xFF);

  writeCommand(0x2B);
  writeData8(y1 >> 8);
  writeData8(y1 & 0xFF);
  writeData8(y2 >> 8);
  writeData8(y2 & 0xFF);

  writeCommand(0x2C);
}

void DevEBox_ILI9341::drawPixel(
  int16_t x, int16_t y, uint16_t color) {

  if (x < 0 || y < 0 ||
      x >= DEVBOX_TFT_WIDTH || y >= DEVBOX_TFT_HEIGHT) {
    return;
  }

  setAddressWindow(x, y, x, y);
  writePixelBus(color);
}

void DevEBox_ILI9341::fillRect(
  int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {

  if (w <= 0 || h <= 0) return;

  // Clip using wider arithmetic to avoid signed overflow.
  int32_t x0 = x;
  int32_t y0 = y;
  int32_t x1 = x0 + w;
  int32_t y1 = y0 + h;

  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > DEVBOX_TFT_WIDTH) x1 = DEVBOX_TFT_WIDTH;
  if (y1 > DEVBOX_TFT_HEIGHT) y1 = DEVBOX_TFT_HEIGHT;

  if (x1 <= x0 || y1 <= y0) return;

  setAddressWindow(x0, y0, x1 - 1, y1 - 1);

  uint32_t totalPixels = (uint32_t)(x1 - x0) * (uint32_t)(y1 - y0);
  while (totalPixels--) {
    writePixelBus(color);
  }
}

void DevEBox_ILI9341::fillScreen(uint16_t color) {
  fillRect(0, 0, DEVBOX_TFT_WIDTH, DEVBOX_TFT_HEIGHT, color);
}

void DevEBox_ILI9341::drawFastHLine(
  int16_t x, int16_t y, int16_t w, uint16_t color) {
  fillRect(x, y, w, 1, color);
}

void DevEBox_ILI9341::drawFastVLine(
  int16_t x, int16_t y, int16_t h, uint16_t color) {
  fillRect(x, y, 1, h, color);
}

void DevEBox_ILI9341::writePixels(
  int16_t x, int16_t y, const uint16_t *pixels, uint16_t count) {

  if (pixels == nullptr || count == 0 ||
      y < 0 || y >= DEVBOX_TFT_HEIGHT) {
    return;
  }

  // This API writes a horizontal run. Clip pixels outside the screen.
  int32_t startX = x;
  int32_t endX = startX + count;

  int32_t clippedStart = startX < 0 ? 0 : startX;
  int32_t clippedEnd = endX > DEVBOX_TFT_WIDTH ? DEVBOX_TFT_WIDTH : endX;

  if (clippedEnd <= clippedStart) return;

  uint16_t sourceOffset = (uint16_t)(clippedStart - startX);
  uint16_t clippedCount = (uint16_t)(clippedEnd - clippedStart);

  setAddressWindow(
    (uint16_t)clippedStart, (uint16_t)y,
    (uint16_t)(clippedEnd - 1), (uint16_t)y
  );

  for (uint16_t i = 0; i < clippedCount; ++i) {
    writePixelBus(pixels[sourceOffset + i]);
  }
}
