
#pragma once

#include <Arduino.h>
#include <SD.h>
#include "DevEBox_ILI9341.h"

/*
 * BMP reader for SD library File objects.
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
 * The reader accepts an already-open File.
 * The caller owns SD.begin(), SD.open(), and File.close().
 *
 * diagnostics may be nullptr.
 */
class DevEBox_BMP {
public:

  // Original interface: native-size rendering
  static bool draw(
    DevEBox_ILI9341 &display,
    File &file,
    Stream *diagnostics = nullptr
  );

  // Scaled rendering
  // scale = 1.0: original size
  // scale = 2.0: twice the dimensions
  // scale = 0.5: half the dimensions
  static bool draw(
    DevEBox_ILI9341 &display,
    File &file,
    float scale,
    Stream *diagnostics = nullptr
  );
};
