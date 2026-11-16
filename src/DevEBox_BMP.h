
#pragma once

#include <Arduino.h>
#include <SdFat.h>

#include "DevEBox_ILI9341.h"

/*
 * BMP reader for SdFat File32 objects.
 *
 * Currently supported:
 *   - 24-bit BI_RGB
 *   - 32-bit BI_RGB
 *   - 32-bit BI_BITFIELDS
 *
 * Supports:
 *   - Bottom-up and top-down BMP images
 *   - Upscaling and downscaling
 *   - Centered rendering
 *   - Screen clipping
 *   - Sequential, sector-aware SD reading
 *
 * The reader accepts an already-open File32.
 * The caller owns SD initialization, file opening, and closing.
 *
 * diagnostics may be nullptr.
 */

class DevEBox_BMP {

public:

  // Native-size rendering
  static bool draw(
    DevEBox_ILI9341 &display,
    File32 &file,
    Stream *diagnostics = nullptr
  );

  // Scaled rendering
  // scale = 1.0: original size
  // scale = 2.0: twice the dimensions
  // scale = 0.5: half the dimensions
  static bool draw(
    DevEBox_ILI9341 &display,
    File32 &file,
    float scale,
    Stream *diagnostics = nullptr
  );
};