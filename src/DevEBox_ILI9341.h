#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include "DevEBox_Config.h"

class DevEBox_ILI9341 : public Adafruit_GFX {
public:
  DevEBox_ILI9341();

  // Initialize the TFT using the default pin map in DevEBox_Config.h.
  void begin();

  // Optional explicit backlight control.
  void setBacklight(bool enabled);

  // Efficiently stream a horizontal run of RGB565 pixels.
  // The pixels are written from x,y toward the right.
  void writePixels(int16_t x, int16_t y,
                   const uint16_t *pixels, uint16_t count);

  void drawPixel(int16_t x, int16_t y, uint16_t color) override;
  void fillRect(int16_t x, int16_t y,
                int16_t w, int16_t h,
                uint16_t color) override;
  void fillScreen(uint16_t color) override;
  void drawFastHLine(int16_t x, int16_t y,
                     int16_t w, uint16_t color) override;
  void drawFastVLine(int16_t x, int16_t y,
                     int16_t h, uint16_t color) override;

private:
  void writeBus(uint16_t value);
  void pulseWR();
  void writeCommand(uint8_t command);
  void writeData8(uint8_t data);
  void writePixelBus(uint16_t color);
  void resetDisplay();
  void initController();
  void setAddressWindow(uint16_t x1, uint16_t y1,
                        uint16_t x2, uint16_t y2);

  uint32_t _dataPins[16];
};
