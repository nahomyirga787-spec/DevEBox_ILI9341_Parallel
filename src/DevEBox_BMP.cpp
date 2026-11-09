
#include "DevEBox_BMP.h"
#include <limits.h>

namespace {

// ==========================================
// LITTLE-ENDIAN READING
// ==========================================

static uint16_t readLE16(const uint8_t *p) {
  return (uint16_t)p[0] |
         ((uint16_t)p[1] << 8);
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

// ==========================================
// CHANNEL EXTRACTION
// ==========================================

static uint8_t extractChannel(uint32_t pixel, uint32_t mask) {

  if (mask == 0) return 0;

  uint8_t shift = 0;

  while (shift < 31 && ((mask >> shift) & 1U) == 0U) {
    ++shift;
  }

  uint32_t value = (pixel & mask) >> shift;
  uint32_t maximum = mask >> shift;

  if (maximum == 0) return 0;

  return (uint8_t)(
    (value * 255UL + maximum / 2) / maximum
  );
}

// ==========================================
// RGB888 TO RGB565
// ==========================================

static uint16_t convertToRGB565(
  uint8_t red,
  uint8_t green,
  uint8_t blue
) {

  return
    ((uint16_t)(red & 0xF8U) << 8) |
    ((uint16_t)(green & 0xFCU) << 3) |
    ((uint16_t)blue >> 3);
}

// ==========================================
// DIAGNOSTICS
// ==========================================

static void logLine(Stream *out, const char *message) {

  if (out) out->println(message);
}

static void logValue(
  Stream *out,
  const char *label,
  uint32_t value
) {

  if (!out) return;

  out->print(label);
  out->println(value);
}

static void logHex(
  Stream *out,
  const char *label,
  uint32_t value
) {

  if (!out) return;

  out->print(label);
  out->print("0x");
  out->println(value, HEX);
}

// ==========================================
// INTEGER CEILING DIVISION
// ==========================================

static uint64_t ceilDivide(
  uint64_t numerator,
  uint64_t denominator
) {

  return (numerator + denominator - 1ULL) / denominator;
}

} // namespace


// ==========================================
// ORIGINAL INTERFACE
// ==========================================

bool DevEBox_BMP::draw(
  DevEBox_ILI9341 &display,
  File &bmp,
  Stream *diagnostics
) {

  return draw(display, bmp, 1.0f, diagnostics);
}


// ==========================================
// SCALED BMP RENDERING
// ==========================================

bool DevEBox_BMP::draw(
  DevEBox_ILI9341 &display,
  File &bmp,
  float scale,
  Stream *diagnostics
) {

  // ----------------------------------------
  // 1. INITIAL VALIDATION
  // ----------------------------------------

  if (!(scale > 0.0f)) {
    logLine(diagnostics, "BMP ERROR: invalid scale.");
    return false;
  }

  if (!bmp) {
    logLine(diagnostics, "BMP ERROR: invalid file handle.");
    return false;
  }

  const int32_t screenWidth = display.width();
  const int32_t screenHeight = display.height();

  if (screenWidth <= 0 || screenHeight <= 0 ||
      screenWidth > DEVBOX_TFT_WIDTH) {

    logLine(diagnostics, "BMP ERROR: unsupported display dimensions.");
    return false;
  }

  const uint32_t actualFileSize = bmp.size();

  uint8_t header[54];

  if (!bmp.seek(0) ||
      bmp.read(header, sizeof(header)) != sizeof(header)) {

    logLine(diagnostics, "BMP ERROR: cannot read BMP header.");
    return false;
  }

  if (header[0] != 'B' || header[1] != 'M') {
    logLine(diagnostics, "BMP ERROR: invalid BMP signature.");
    return false;
  }

  // ----------------------------------------
  // 2. READ BMP HEADER
  // ----------------------------------------

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

  if (width <= 0 ||
      signedHeight == 0 ||
      signedHeight == INT32_MIN) {

    logLine(diagnostics, "BMP ERROR: invalid dimensions.");
    return false;
  }

  if (planes != 1) {
    logLine(diagnostics, "BMP ERROR: color planes must equal one.");
    return false;
  }

  const bool topDown = signedHeight < 0;

  const uint32_t height = topDown
    ? (uint32_t)(-signedHeight)
    : (uint32_t)signedHeight;

  // ----------------------------------------
  // 3. CALCULATE SCALED DIMENSIONS
  // ----------------------------------------

  const double scaledWidthValue =
    (double)width * (double)scale;

  const double scaledHeightValue =
    (double)height * (double)scale;

  if (scaledWidthValue < 0.5 ||
      scaledHeightValue < 0.5 ||
      scaledWidthValue > (double)INT32_MAX - 0.5 ||
      scaledHeightValue > (double)INT32_MAX - 0.5) {

    logLine(diagnostics, "BMP ERROR: invalid scaled dimensions.");
    return false;
  }

  const uint32_t scaledWidth =
    (uint32_t)(scaledWidthValue + 0.5);

  const uint32_t scaledHeight =
    (uint32_t)(scaledHeightValue + 0.5);

  // ----------------------------------------
  // 4. FORMAT VALIDATION
  // ----------------------------------------

  const bool supported =
    (bitsPerPixel == 24 && compression == 0) ||
    (bitsPerPixel == 32 &&
      (compression == 0 || compression == 3));

  if (!supported) {
    logLine(diagnostics, "BMP ERROR: unsupported format.");
    logLine(diagnostics,
      "Supported: 24-bit BI_RGB, 32-bit BI_RGB/BI_BITFIELDS.");
    return false;
  }

  // ----------------------------------------
  // 5. DIAGNOSTICS
  // ----------------------------------------

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

    logValue(diagnostics, "Scale factor x1000: ",
      (uint32_t)(scale * 1000.0f));

    logValue(diagnostics, "Scaled width: ", scaledWidth);
    logValue(diagnostics, "Scaled height: ", scaledHeight);
  }

  // ----------------------------------------
  // 6. RGB MASKS
  // ----------------------------------------

  uint32_t redMask = 0x00FF0000UL;
  uint32_t greenMask = 0x0000FF00UL;
  uint32_t blueMask = 0x000000FFUL;

  if (bitsPerPixel == 32 && compression == 3) {

    const uint32_t maskOffset = 14UL + 40UL;

    if ((uint64_t)maskOffset + 12ULL > pixelOffset ||
        !bmp.seek(maskOffset)) {

      logLine(diagnostics, "BMP ERROR: invalid RGB masks.");
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
      logLine(diagnostics, "BMP ERROR: invalid RGB masks.");
      return false;
    }
  }

  // ----------------------------------------
  // 7. CALCULATE SOURCE ROW SIZE
  // ----------------------------------------

  const uint32_t bytesPerPixel = bitsPerPixel / 8U;

  const uint64_t pixelDataBytes64 =
    (uint64_t)(uint32_t)width * bytesPerPixel;

  const uint64_t rowSize64 =
    (pixelDataBytes64 + 3ULL) & ~3ULL;

  const uint64_t imageBytes =
    rowSize64 * height;

  const uint64_t expectedEnd =
    (uint64_t)pixelOffset + imageBytes;

  if (pixelDataBytes64 > UINT32_MAX ||
      rowSize64 > UINT32_MAX ||
      pixelOffset > actualFileSize ||
      expectedEnd > actualFileSize) {

    logLine(diagnostics, "BMP ERROR: pixel data exceeds file bounds.");
    return false;
  }

  const uint32_t pixelDataBytes =
    (uint32_t)pixelDataBytes64;

  const uint32_t rowSize =
    (uint32_t)rowSize64;

  logValue(diagnostics, "Bytes per pixel: ", bytesPerPixel);
  logValue(diagnostics, "Row size: ", rowSize);

  // ----------------------------------------
  // 8. CALCULATE SCREEN POSITION
  // ----------------------------------------

  const int32_t xStart =
    ((int32_t)screenWidth - (int32_t)scaledWidth) / 2;

  const int32_t yStart =
    ((int32_t)screenHeight - (int32_t)scaledHeight) / 2;

  // Visible portion of the scaled image.

  const uint32_t visibleXStart =
    xStart < 0 ? (uint32_t)(-(int64_t)xStart) : 0;

  const uint32_t visibleYStart =
    yStart < 0 ? (uint32_t)(-(int64_t)yStart) : 0;

  const int64_t visibleXEnd64 =
    (int64_t)screenWidth - xStart;

  const int64_t visibleYEnd64 =
    (int64_t)screenHeight - yStart;

  const uint32_t visibleXEnd =
    visibleXEnd64 < (int64_t)scaledWidth
      ? (uint32_t)visibleXEnd64
      : scaledWidth;

  const uint32_t visibleYEnd =
    visibleYEnd64 < (int64_t)scaledHeight
      ? (uint32_t)visibleYEnd64
      : scaledHeight;

  if (visibleXStart >= visibleXEnd ||
      visibleYStart >= visibleYEnd) {

    logLine(diagnostics, "BMP ERROR: image is outside display bounds.");
    return false;
  }

  const uint16_t visibleWidth =
    (uint16_t)(visibleXEnd - visibleXStart);

  const int16_t drawX =
    (int16_t)(xStart + (int32_t)visibleXStart);

  // ----------------------------------------
  // 9. FIXED-SIZE STREAMING BUFFERS
  // ----------------------------------------

  // Only 64 bytes are read from SD at a time.
  uint8_t chunkBuffer[64];

  // Holds one incomplete or complete source pixel.
  uint8_t pixelBuffer[4];

  // Holds only the visible RGB565 output row.
  uint16_t colorRow[DEVBOX_TFT_WIDTH];

  // ----------------------------------------
  // 10. SEEK TO PIXEL DATA
  // ----------------------------------------

  if (!bmp.seek(pixelOffset)) {
    logLine(diagnostics, "BMP ERROR: cannot seek to pixel data.");
    return false;
  }

  logLine(diagnostics, "Reading BMP sequentially...");

  // ----------------------------------------
  // 11. PROCESS SOURCE ROWS
  // ----------------------------------------

  for (uint32_t sourceRow = 0;
       sourceRow < height;
       ++sourceRow) {

    uint32_t rowBytesRemaining = rowSize;
    uint32_t pixelBytesRemaining = pixelDataBytes;

    uint32_t sourceX = 0;

    uint8_t pixelByteCount = 0;

    // Read one row in small, sector-aware chunks.
    while (rowBytesRemaining > 0) {

      const uint32_t filePosition = bmp.position();

      const uint32_t bytesToSectorEnd =
        512UL - (filePosition % 512UL);

      uint32_t chunkSize = rowBytesRemaining;

      if (chunkSize > sizeof(chunkBuffer)) {
        chunkSize = sizeof(chunkBuffer);
      }

      if (chunkSize > bytesToSectorEnd) {
        chunkSize = bytesToSectorEnd;
      }

      const int bytesRead =
        bmp.read(chunkBuffer, chunkSize);

      if (bytesRead <= 0) {

        if (diagnostics) {
          logLine(diagnostics, "BMP ERROR: row read failed.");

          logValue(diagnostics, "Source row: ", sourceRow);

          logValue(diagnostics, "Bytes received: ",
            rowSize - rowBytesRemaining);

          logValue(diagnostics, "Expected row bytes: ", rowSize);

          logValue(diagnostics, "File position: ", bmp.position());
        }

        return false;
      }

      rowBytesRemaining -= (uint32_t)bytesRead;

      // ------------------------------------
      // PROCESS BYTES INSIDE THIS CHUNK
      // ------------------------------------

      for (int i = 0; i < bytesRead; ++i) {

        // Ignore BMP row-padding bytes.
        if (pixelBytesRemaining == 0) {
          continue;
        }

        pixelBuffer[pixelByteCount++] = chunkBuffer[i];

        --pixelBytesRemaining;

        // Wait until we have a complete pixel.
        if (pixelByteCount < bytesPerPixel) {
          continue;
        }

        uint8_t red;
        uint8_t green;
        uint8_t blue;

        // ----------------------------------
        // DECODE SOURCE PIXEL
        // ----------------------------------

        if (bitsPerPixel == 24) {

          blue  = pixelBuffer[0];
          green = pixelBuffer[1];
          red   = pixelBuffer[2];

        } else {

          const uint32_t pixel =
            (uint32_t)pixelBuffer[0] |
            ((uint32_t)pixelBuffer[1] << 8) |
            ((uint32_t)pixelBuffer[2] << 16) |
            ((uint32_t)pixelBuffer[3] << 24);

          red = extractChannel(pixel, redMask);
          green = extractChannel(pixel, greenMask);
          blue = extractChannel(pixel, blueMask);
        }

        const uint16_t color =
          convertToRGB565(red, green, blue);

        // ----------------------------------
        // HORIZONTAL NEAREST-NEIGHBOR SCALE
        // ----------------------------------

        const uint64_t firstOutX =
          ceilDivide((uint64_t)sourceX * scaledWidth,
                     (uint32_t)width);

        const uint64_t endOutX =
          ceilDivide((uint64_t)(sourceX + 1ULL) * scaledWidth,
                     (uint32_t)width);

        uint64_t clippedStart = firstOutX;

        if (clippedStart < visibleXStart) {
          clippedStart = visibleXStart;
        }

        uint64_t clippedEnd = endOutX;

        if (clippedEnd > visibleXEnd) {
          clippedEnd = visibleXEnd;
        }

        for (uint64_t outX = clippedStart;
             outX < clippedEnd;
             ++outX) {

          colorRow[outX - visibleXStart] = color;
        }

        ++sourceX;
        pixelByteCount = 0;
      }
    }

    // Verify that the entire source row was decoded.
    if (sourceX != (uint32_t)width ||
        pixelByteCount != 0 ||
        pixelBytesRemaining != 0) {

      logLine(diagnostics, "BMP ERROR: incomplete pixel row.");
      return false;
    }

    // --------------------------------------
    // 12. DETERMINE DISPLAY-ORIENTED ROW
    // --------------------------------------

    const uint32_t destinationSourceRow =
      topDown
        ? sourceRow
        : height - 1U - sourceRow;

    // --------------------------------------
    // 13. VERTICAL NEAREST-NEIGHBOR SCALE
    // --------------------------------------

    const uint64_t firstOutY =
      ceilDivide(
        (uint64_t)destinationSourceRow * scaledHeight,
        height
      );

    const uint64_t endOutY =
      ceilDivide(
        (uint64_t)(destinationSourceRow + 1ULL) * scaledHeight,
        height
      );

    uint64_t clippedYStart = firstOutY;

    if (clippedYStart < visibleYStart) {
      clippedYStart = visibleYStart;
    }

    uint64_t clippedYEnd = endOutY;

    if (clippedYEnd > visibleYEnd) {
      clippedYEnd = visibleYEnd;
    }

    // A source row can produce multiple output rows
    // during upscaling, or no output rows during downscaling.

    for (uint64_t outY = clippedYStart;
         outY < clippedYEnd;
         ++outY) {

      const int16_t drawY =
        (int16_t)(yStart + (int32_t)outY);

      display.writePixels(
        drawX,
        drawY,
        colorRow,
        visibleWidth
      );
    }

    if (diagnostics && ((sourceRow + 1U) % 40U == 0U)) {
      logValue(diagnostics, "Rows processed: ", sourceRow + 1U);
    }
  }

  logLine(diagnostics, "BMP DISPLAY SUCCESSFUL!");

  return true;
}
