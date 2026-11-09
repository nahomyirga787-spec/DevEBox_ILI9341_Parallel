#include "DevEBox_BMP.h"
#include <limits.h>

namespace {

static uint16_t readLE16(const uint8_t *p) {
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t readLE32(const uint8_t *p) {
  return (uint32_t)p[0] |
         ((uint32_t)p[1] << 8) |
         ((uint32_t)p[2] << 16) |
         ((uint32_t)p[3] << 24);
}

static int32_t readLES32(const uint8_t *p) {
  return (int32_t)readLE32(p);
}

static uint8_t extractChannel(uint32_t pixel, uint32_t mask) {
  if (mask == 0) return 0;

  uint8_t shift = 0;
  while (shift < 31 && ((mask >> shift) & 1U) == 0U) {
    ++shift;
  }

  uint32_t value = (pixel & mask) >> shift;
  uint32_t maximum = mask >> shift;
  if (maximum == 0) return 0;

  return (uint8_t)((value * 255UL + maximum / 2) / maximum);
}

static void logLine(Stream *out, const char *message) {
  if (out) out->println(message);
}

static void logValue(Stream *out, const char *label, uint32_t value) {
  if (!out) return;
  out->print(label);
  out->println(value);
}

static void logHex(Stream *out, const char *label, uint32_t value) {
  if (!out) return;
  out->print(label);
  out->print("0x");
  out->println(value, HEX);
}

} // namespace

bool DevEBox_BMP::draw(
  DevEBox_ILI9341 &display,
  File &bmp,
  Stream *diagnostics) {

  if (!bmp) {
    logLine(diagnostics, "BMP ERROR: invalid file handle.");
    return false;
  }

  const uint32_t actualFileSize = bmp.size();
  uint8_t header[54];

  if (!bmp.seek(0) || bmp.read(header, sizeof(header)) != sizeof(header)) {
    logLine(diagnostics, "BMP ERROR: cannot read BMP header.");
    return false;
  }

  if (header[0] != 'B' || header[1] != 'M') {
    logLine(diagnostics, "BMP ERROR: invalid BMP signature.");
    return false;
  }

  const uint32_t declaredFileSize = readLE32(&header[2]);
  const uint32_t pixelOffset = readLE32(&header[10]);
  const uint32_t dibSize = readLE32(&header[14]);
  const int32_t width = readLES32(&header[18]);
  const int32_t signedHeight = readLES32(&header[22]);
  const uint16_t planes = readLE16(&header[26]);
  const uint16_t bitsPerPixel = readLE16(&header[28]);
  const uint32_t compression = readLE32(&header[30]);

  if (dibSize < 40) {
    logLine(diagnostics, "BMP ERROR: unsupported DIB header.");
    return false;
  }

  if (width <= 0 || signedHeight == 0 || signedHeight == INT32_MIN) {
    logLine(diagnostics, "BMP ERROR: invalid dimensions.");
    return false;
  }

  const bool topDown = signedHeight < 0;
  const uint32_t height = topDown
    ? (uint32_t)(-signedHeight)
    : (uint32_t)signedHeight;

  if (planes != 1) {
    logLine(diagnostics, "BMP ERROR: color planes must equal one.");
    return false;
  }

  if ((uint32_t)width > display.width() ||
      height > (uint32_t)display.height()) {
    logLine(diagnostics, "BMP ERROR: image is larger than display.");
    return false;
  }

  const bool supported =
    (bitsPerPixel == 24 && compression == 0) ||
    (bitsPerPixel == 32 && (compression == 0 || compression == 3));

  if (!supported) {
    logLine(diagnostics, "BMP ERROR: unsupported format.");
    logLine(diagnostics, "Supported: 24-bit BI_RGB, 32-bit BI_RGB/BI_BITFIELDS.");
    return false;
  }

  if (diagnostics) {
    logLine(diagnostics, "BMP HEADER INFORMATION:");
    logValue(diagnostics, "DIB size: ", dibSize);
    logValue(diagnostics, "Width: ", (uint32_t)width);
    logValue(diagnostics, "Height: ", height);
    logValue(diagnostics, "Bits per pixel: ", bitsPerPixel);
    logValue(diagnostics, "Compression: ", compression);
    logValue(diagnostics, "Pixel offset: ", pixelOffset);
    logValue(diagnostics, "Actual file size: ", actualFileSize);
    logValue(diagnostics, "Declared file size: ", declaredFileSize);
  }

  uint32_t redMask = 0x00FF0000UL;
  uint32_t greenMask = 0x0000FF00UL;
  uint32_t blueMask = 0x000000FFUL;

  if (bitsPerPixel == 32 && compression == 3) {
    // BITMAPV2/V3/V4/V5 headers (DIB >= 52) store masks
    // immediately after the first 40 bytes of the DIB header.
    // A 40-byte BITMAPINFOHEADER stores masks after the DIB.
    const uint32_t maskOffset = 14UL + 40UL;

    if ((uint64_t)maskOffset + 12UL > pixelOffset ||
        !bmp.seek(maskOffset)) {
      logLine(diagnostics, "BMP ERROR: invalid or inaccessible RGB masks.");
      return false;
    }

    uint8_t masks[12];
    if (bmp.read(masks, sizeof(masks)) != sizeof(masks)) {
      logLine(diagnostics, "BMP ERROR: cannot read RGB masks.");
      return false;
    }

    redMask = readLE32(&masks[0]);
    greenMask = readLE32(&masks[4]);
    blueMask = readLE32(&masks[8]);

    logHex(diagnostics, "Red mask: ", redMask);
    logHex(diagnostics, "Green mask: ", greenMask);
    logHex(diagnostics, "Blue mask: ", blueMask);

    if (redMask == 0 || greenMask == 0 || blueMask == 0) {
      logLine(diagnostics, "BMP ERROR: one or more RGB masks are zero.");
      return false;
    }
  }

  const uint32_t bytesPerPixel = bitsPerPixel / 8U;
  const uint64_t rowSize64 =
    (((uint64_t)(uint32_t)width * bytesPerPixel) + 3ULL) & ~3ULL;
  const uint64_t imageBytes = rowSize64 * height;
  const uint64_t expectedEnd = (uint64_t)pixelOffset + imageBytes;

  if (rowSize64 > (uint64_t)display.width() * 4ULL ||
      pixelOffset > actualFileSize ||
      expectedEnd > actualFileSize) {
    logLine(diagnostics, "BMP ERROR: pixel data exceeds file bounds.");
    return false;
  }

  const uint32_t rowSize = (uint32_t)rowSize64;
  logValue(diagnostics, "Bytes per pixel: ", bytesPerPixel);
  logValue(diagnostics, "Row size: ", rowSize);
  logValue(diagnostics, "Expected pixel bytes: ", (uint32_t)imageBytes);

  // Fixed-size buffers sized for the configured 320-pixel display.
  // The row bytes and converted RGB565 row are kept separately.
  uint8_t rowBuffer[DEVBOX_TFT_WIDTH * 4];
  uint16_t colorRow[DEVBOX_TFT_WIDTH];

  const uint16_t xStart = (display.width() - (int16_t)width) / 2;
  const uint16_t yStart = (display.height() - (int16_t)height) / 2;

  if (!bmp.seek(pixelOffset)) {
    logLine(diagnostics, "BMP ERROR: cannot seek to pixel data.");
    return false;
  }

  logLine(diagnostics, "Reading BMP sequentially...");

  for (uint32_t sourceRow = 0; sourceRow < height; ++sourceRow) {
    uint32_t received = 0;

    while (received < rowSize) {
      const uint32_t remaining = rowSize - received;
      const uint32_t filePosition = bmp.position();
      const uint32_t bytesToSectorEnd = 512UL - (filePosition % 512UL);

      uint32_t chunkSize = remaining;
      if (chunkSize > 64U) chunkSize = 64U;
      if (chunkSize > bytesToSectorEnd) chunkSize = bytesToSectorEnd;

      const int bytesRead = bmp.read(&rowBuffer[received], chunkSize);

      if (bytesRead <= 0) {
        if (diagnostics) {
          logLine(diagnostics, "BMP ERROR: row read failed.");
          logValue(diagnostics, "Source row: ", sourceRow);
          logValue(diagnostics, "Bytes received: ", received);
          logValue(diagnostics, "Expected row bytes: ", rowSize);
          logValue(diagnostics, "File position: ", bmp.position());
        }
        return false;
      }

      received += (uint32_t)bytesRead;
    }

    const uint32_t destinationRow =
      topDown ? sourceRow : (height - 1U - sourceRow);

    for (uint32_t x = 0; x < (uint32_t)width; ++x) {
      uint8_t red, green, blue;

      if (bitsPerPixel == 24) {
        blue  = rowBuffer[x * 3U];
        green = rowBuffer[x * 3U + 1U];
        red   = rowBuffer[x * 3U + 2U];
      } else {
        const uint32_t pixel =
          (uint32_t)rowBuffer[x * 4U] |
          ((uint32_t)rowBuffer[x * 4U + 1U] << 8) |
          ((uint32_t)rowBuffer[x * 4U + 2U] << 16) |
          ((uint32_t)rowBuffer[x * 4U + 3U] << 24);

        red = extractChannel(pixel, redMask);
        green = extractChannel(pixel, greenMask);
        blue = extractChannel(pixel, blueMask);
      }

      colorRow[x] =
        ((uint16_t)(red & 0xF8U) << 8) |
        ((uint16_t)(green & 0xFCU) << 3) |
        ((uint16_t)blue >> 3);
    }

    display.writePixels(
      xStart,
      yStart + (int16_t)destinationRow,
      colorRow,
      (uint16_t)width
    );

    if (diagnostics && ((sourceRow + 1U) % 40U == 0U)) {
      logValue(diagnostics, "Rows processed: ", sourceRow + 1U);
    }
  }

  logLine(diagnostics, "BMP DISPLAY SUCCESSFUL!");
  return true;
}
