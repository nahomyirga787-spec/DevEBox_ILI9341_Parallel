#pragma once

#include <Arduino.h>

class DevEBox_IDCT {

public:

  // Transform 64 dequantized DCT coefficients
  // into 64 unsigned 8-bit pixel values.
  //
  // Input:
  //   64 signed dequantized coefficients in natural order.
  //
  // Output:
  //   8x8 pixel block in row-major order.
  //
  // Returns false if either pointer is NULL.
  static bool transform(
    const int32_t coefficients[64],
    uint8_t pixels[64]
  );

private:

  static int64_t roundDivide(
    int64_t numerator,
    int64_t denominator
  );

  static uint8_t clampPixel(int64_t value);
};
