#pragma once

#include <Arduino.h>
#include "DevEBox_JPEG.h"

// Converts one decoded JPEG MCU into row-major RGB565 pixels.
//
// Supported color layouts:
//   - Grayscale (one component)
//   - Three-component JFIF-style YCbCr with component IDs 1, 2, 3
//     and 4:4:4, 4:2:2, or 4:2:0 sampling.
//
// Chroma upsampling uses nearest-neighbor replication. This is simple,
// deterministic, and suitable for the first display implementation.
class DevEBox_JPEG_Color {

public:

  static const uint16_t MAX_MCU_PIXELS = 256; // 16 x 16

  // Convert an MCU into RGB565.
  //
  // rgb565 must have room for at least MAX_MCU_PIXELS values.
  // outputWidth and outputHeight report the actual MCU dimensions.
  // Pixels outside the image boundary are still reconstructed; the
  // display stage must clip them against the JPEG image dimensions.
  static bool convertMCU(
    const DevEBox_JPEG::ImageInfo &imageInfo,
    const DevEBox_JPEG::MCU &mcu,
    uint16_t *rgb565,
    uint16_t outputCapacity,
    uint8_t &outputWidth,
    uint8_t &outputHeight
  );

  // Convert one YCbCr pixel to RGB565. Cb and Cr are centered at 128.
  static uint16_t ycbcrToRGB565(
    uint8_t y,
    uint8_t cb,
    uint8_t cr
  );

  // Convert one grayscale sample to RGB565.
  static uint16_t grayscaleToRGB565(uint8_t gray);

private:

  static int32_t roundQ16(int32_t value);
  static uint8_t clamp8(int32_t value);
};
