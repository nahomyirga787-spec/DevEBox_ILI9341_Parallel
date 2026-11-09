#pragma once

#include <Arduino.h>
#include <SD.h>
#include "DevEBox_ILI9341.h"

/*
 * BMP reader for SD library File objects.
 *
 * Supported formats:
 *   - 24-bit BI_RGB (uncompressed)
 *   - 32-bit BI_RGB (uncompressed)
 *   - 32-bit BI_BITFIELDS
 *
 * The reader accepts an already-open File. The caller owns SD.begin(),
 * SD.open(), and File.close().
 *
 * diagnostics may be nullptr. If provided, messages are written to it.
 */
class DevEBox_BMP {
public:
  static bool draw(DevEBox_ILI9341 &display,
                   File &file,
                   Stream *diagnostics = nullptr);
};
