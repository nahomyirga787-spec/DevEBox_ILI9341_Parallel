
#pragma once

#include <Arduino.h>
#include <SdFat.h>

#include "DevEBox_ILI9341.h"
#include "DevEBox_JPEG.h"
#include "DevEBox_JPEG_Color.h"

class DevEBox_JPEG_Display {

public:

  static bool draw(
    DevEBox_ILI9341 &tft,
    File32 &file,
    float scale = 1.0f,
    Stream *diagnostics = nullptr
  );

private:

  DevEBox_JPEG decoder_;

  DevEBox_JPEG::MCU mcu_;

  uint16_t mcuPixels_[DevEBox_JPEG_Color::MAX_MCU_PIXELS];

  uint16_t lineBuffer_[320];

  bool drawInternal(
    DevEBox_ILI9341 &tft,
    File32 &file,
    float scale,
    Stream *diagnostics
  );

  uint32_t ceilDivide(
    uint64_t numerator,
    uint32_t denominator
  );

  uint16_t mapOutputToSource(
    uint32_t outputCoordinate,
    uint16_t sourceLength,
    uint32_t outputLength
  );

  bool renderMCU(
    DevEBox_ILI9341 &tft,
    const uint16_t *pixels,
    uint8_t sourceMCUWidth,
    uint8_t sourceMCUHeight,
    uint16_t mcuX,
    uint16_t mcuY,
    uint16_t sourceImageWidth,
    uint16_t sourceImageHeight,
    uint32_t outputImageWidth,
    uint32_t outputImageHeight,
    int32_t displayOffsetX,
    int32_t displayOffsetY
  );
};
