#include "DevEBox_IDCT.h"

// ======================================================
// FIXED-POINT IDCT
// ======================================================
//
// Q14 precision:
// 1.0 = 16384
//
// Matrix:
// T[x][u] = C(u) * cos((2x + 1) * u * PI / 16)
//
// C(0) = 1 / sqrt(2)
// C(u) = 1 for u > 0
//
// ======================================================

static const int32_t IDCT_MATRIX[8][8] = {

  { 11585,  16069,  15137,  13623,  11585,   9102,   6270,   3196 },

  { 11585,  13623,   6270,  -3196, -11585, -16069, -15137,  -9102 },

  { 11585,   9102,  -6270, -16069, -11585,   3196,  15137,  13623 },

  { 11585,   3196, -15137,  -9102,  11585,  13623,  -6270, -16069 },

  { 11585,  -3196, -15137,   9102,  11585, -13623,  -6270,  16069 },

  { 11585,  -9102,  -6270,  16069, -11585,  -3196,  15137, -13623 },

  { 11585, -13623,   6270,   3196, -11585,  16069, -15137,   9102 },

  { 11585, -16069,  15137, -13623,  11585,  -9102,   6270,  -3196 }

};

// Q14 * Q14 * 4
static const int64_t IDCT_DIVISOR =
  4LL * 16384LL * 16384LL;


// ======================================================
// SIGNED ROUNDING DIVISION
// ======================================================

int64_t DevEBox_IDCT::roundDivide(
  int64_t numerator,
  int64_t denominator
) {

  if (numerator >= 0) {
    return (numerator + denominator / 2) / denominator;
  }

  return -((-numerator + denominator / 2) / denominator);
}


// ======================================================
// PIXEL CLAMPING
// ======================================================

uint8_t DevEBox_IDCT::clampPixel(int64_t value) {

  if (value < 0) {
    return 0;
  }

  if (value > 255) {
    return 255;
  }

  return (uint8_t)value;
}


// ======================================================
// 8x8 INVERSE DISCRETE COSINE TRANSFORM
// ======================================================

bool DevEBox_IDCT::transform(
  const int32_t coefficients[64],
  uint8_t pixels[64]
) {

  if (coefficients == nullptr || pixels == nullptr) {
    return false;
  }

  // PASS 1: horizontal inverse transform.
  // Intermediate values retain Q14 precision.
  int64_t temporary[64];

  for (uint8_t v = 0; v < 8; v++) {

    for (uint8_t x = 0; x < 8; x++) {

      int64_t sum = 0;

      for (uint8_t u = 0; u < 8; u++) {

        int32_t coefficient =
          coefficients[v * 8 + u];

        sum +=
          (int64_t)coefficient *
          IDCT_MATRIX[x][u];

      }

      temporary[v * 8 + x] = sum;

    }

  }

  // PASS 2: vertical inverse transform.
  for (uint8_t y = 0; y < 8; y++) {

    for (uint8_t x = 0; x < 8; x++) {

      int64_t sum = 0;

      for (uint8_t v = 0; v < 8; v++) {

        sum +=
          temporary[v * 8 + x] *
          IDCT_MATRIX[y][v];

      }

      int64_t reconstructed =
        roundDivide(sum, IDCT_DIVISOR);

      // JPEG level shift.
      reconstructed += 128;

      pixels[y * 8 + x] =
        clampPixel(reconstructed);

    }

  }

  return true;
}
