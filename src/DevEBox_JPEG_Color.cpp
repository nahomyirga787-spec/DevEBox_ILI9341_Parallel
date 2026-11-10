#include "DevEBox_JPEG_Color.h"

// ============================================================
// FIXED-POINT ROUNDING AND CLAMPING
// ============================================================

int32_t DevEBox_JPEG_Color::roundQ16(int32_t value) {

  if (value >= 0) {
    return (value + 32768L) / 65536L;
  }

  return -(((-value) + 32768L) / 65536L);
}


uint8_t DevEBox_JPEG_Color::clamp8(int32_t value) {

  if (value < 0) {
    return 0;
  }

  if (value > 255) {
    return 255;
  }

  return (uint8_t)value;
}


// ============================================================
// GRAYSCALE TO RGB565
// ============================================================

uint16_t DevEBox_JPEG_Color::grayscaleToRGB565(uint8_t gray) {

  const uint16_t red = (uint16_t)(gray >> 3);
  const uint16_t green = (uint16_t)(gray >> 2);
  const uint16_t blue = (uint16_t)(gray >> 3);

  return (red << 11) | (green << 5) | blue;
}


// ============================================================
// YCbCr TO RGB565
// ============================================================

uint16_t DevEBox_JPEG_Color::ycbcrToRGB565(
  uint8_t y,
  uint8_t cb,
  uint8_t cr
) {

  const int32_t cbDifference = (int32_t)cb - 128;
  const int32_t crDifference = (int32_t)cr - 128;

  // ITU-R BT.601/JFIF-style full-range JPEG conversion.
  // Coefficients are represented in Q16 fixed-point format.
  const int32_t red =
    (int32_t)y + roundQ16(91881L * crDifference);

  const int32_t green =
    (int32_t)y - roundQ16(
      22554L * cbDifference +
      46802L * crDifference
    );

  const int32_t blue =
    (int32_t)y + roundQ16(116130L * cbDifference);

  const uint8_t r = clamp8(red);
  const uint8_t g = clamp8(green);
  const uint8_t b = clamp8(blue);

  return (
    ((uint16_t)(r >> 3) << 11) |
    ((uint16_t)(g >> 2) << 5)  |
    ((uint16_t)(b >> 3))
  );
}


// ============================================================
// FIND A BLOCK'S PIXEL SAMPLE
// ============================================================

static bool getComponentSample(
  const DevEBox_JPEG::MCU &mcu,
  const int8_t blockMap[DevEBox_JPEG::MAX_COMPONENTS][4],
  uint8_t componentIndex,
  uint8_t horizontalSampling,
  uint8_t sampleX,
  uint8_t sampleY,
  uint8_t &sample
) {

  const uint8_t blockX = sampleX >> 3;
  const uint8_t blockY = sampleY >> 3;

  if (blockX >= horizontalSampling || blockY >= 2) {
    return false;
  }

  const uint8_t slot = blockY * horizontalSampling + blockX;
  const int8_t blockIndex = blockMap[componentIndex][slot];

  if (blockIndex < 0 || (uint8_t)blockIndex >= mcu.blockCount) {
    return false;
  }

  const uint8_t pixelX = sampleX & 7;
  const uint8_t pixelY = sampleY & 7;

  sample = mcu.blocks[(uint8_t)blockIndex].pixels[
    pixelY * 8 + pixelX
  ];

  return true;
}


// ============================================================
// CONVERT ONE MCU TO RGB565
// ============================================================

bool DevEBox_JPEG_Color::convertMCU(
  const DevEBox_JPEG::ImageInfo &imageInfo,
  const DevEBox_JPEG::MCU &mcu,
  uint16_t *rgb565,
  uint16_t outputCapacity,
  uint8_t &outputWidth,
  uint8_t &outputHeight
) {

  outputWidth = 0;
  outputHeight = 0;

  if (rgb565 == nullptr ||
      imageInfo.componentCount == 0 ||
      imageInfo.componentCount > DevEBox_JPEG::MAX_COMPONENTS ||
      mcu.blockCount == 0 ||
      mcu.blockCount > DevEBox_JPEG::MAX_MCU_BLOCKS) {
    return false;
  }

  uint8_t maximumH = 0;
  uint8_t maximumV = 0;

  for (uint8_t i = 0; i < imageInfo.componentCount; i++) {

    const DevEBox_JPEG::Component &component =
      imageInfo.components[i];

    if (component.horizontalSampling == 0 ||
        component.verticalSampling == 0) {
      return false;
    }

    if (component.horizontalSampling > maximumH) {
      maximumH = component.horizontalSampling;
    }

    if (component.verticalSampling > maximumV) {
      maximumV = component.verticalSampling;
    }
  }

  // This first color stage intentionally targets the common layouts
  // 4:4:4, 4:2:2, and 4:2:0. Their MCU dimensions are at most 16x16.
  if (maximumH > 2 || maximumV > 2) {
    return false;
  }

  const uint8_t width = maximumH * 8;
  const uint8_t height = maximumV * 8;
  const uint16_t pixelCount = (uint16_t)width * height;

  if (pixelCount > MAX_MCU_PIXELS || outputCapacity < pixelCount) {
    return false;
  }

  // Each component can occupy at most a 2x2 grid of 8x8 blocks.
  // -1 means that the corresponding block was not found.
  int8_t blockMap[DevEBox_JPEG::MAX_COMPONENTS][4];

  for (uint8_t c = 0; c < DevEBox_JPEG::MAX_COMPONENTS; c++) {
    for (uint8_t i = 0; i < 4; i++) {
      blockMap[c][i] = -1;
    }
  }

  for (uint8_t i = 0; i < mcu.blockCount; i++) {

    const DevEBox_JPEG::MCUBlock &block = mcu.blocks[i];

    if (block.componentIndex >= imageInfo.componentCount) {
      return false;
    }

    const DevEBox_JPEG::Component &component =
      imageInfo.components[block.componentIndex];

    if (block.blockX >= component.horizontalSampling ||
        block.blockY >= component.verticalSampling ||
        block.blockY >= 2) {
      return false;
    }

    const uint8_t slot =
      block.blockY * component.horizontalSampling + block.blockX;

    if (slot >= 4 || blockMap[block.componentIndex][slot] >= 0) {
      return false;
    }

    blockMap[block.componentIndex][slot] = (int8_t)i;
  }

  // Verify that every expected component block is present.
  uint8_t expectedBlocks = 0;

  for (uint8_t c = 0; c < imageInfo.componentCount; c++) {

    const DevEBox_JPEG::Component &component =
      imageInfo.components[c];

    expectedBlocks +=
      component.horizontalSampling * component.verticalSampling;

    for (uint8_t by = 0; by < component.verticalSampling; by++) {
      for (uint8_t bx = 0; bx < component.horizontalSampling; bx++) {

        const uint8_t slot = by * component.horizontalSampling + bx;

        if (slot >= 4 || blockMap[c][slot] < 0) {
          return false;
        }
      }
    }
  }

  if (expectedBlocks != mcu.blockCount) {
    return false;
  }

  // For three-component JPEG, identify JFIF Y, Cb, and Cr by IDs.
  // This avoids assuming that frame component storage is always
  // in the same order as the color channels.
  int8_t yIndex = -1;
  int8_t cbIndex = -1;
  int8_t crIndex = -1;

  if (imageInfo.componentCount == 3) {

    for (uint8_t i = 0; i < imageInfo.componentCount; i++) {

      const uint8_t id = imageInfo.components[i].id;

      if (id == 1) yIndex = (int8_t)i;
      else if (id == 2) cbIndex = (int8_t)i;
      else if (id == 3) crIndex = (int8_t)i;
    }

    if (yIndex < 0 || cbIndex < 0 || crIndex < 0) {
      return false;
    }
  }

  // Render each output pixel. Sampling coordinates are mapped back
  // into each component's lower-resolution sample plane. This performs
  // nearest-neighbor chroma upsampling for 4:2:2 and 4:2:0.
  for (uint8_t y = 0; y < height; y++) {

    for (uint8_t x = 0; x < width; x++) {

      uint8_t ySample;

      const DevEBox_JPEG::Component &yComponent =
        imageInfo.components[(imageInfo.componentCount == 1)
          ? 0 : (uint8_t)yIndex];

      const uint8_t ySampleX = (uint8_t)(
        ((uint16_t)x * yComponent.horizontalSampling) / maximumH
      );

      const uint8_t ySampleY = (uint8_t)(
        ((uint16_t)y * yComponent.verticalSampling) / maximumV
      );

      if (!getComponentSample(
            mcu,
            blockMap,
            (imageInfo.componentCount == 1) ? 0 : (uint8_t)yIndex,
            yComponent.horizontalSampling,
            ySampleX,
            ySampleY,
            ySample
          )) {
        return false;
      }

      if (imageInfo.componentCount == 1) {
        rgb565[(uint16_t)y * width + x] =
          grayscaleToRGB565(ySample);
      }
      else {

        const DevEBox_JPEG::Component &cbComponent =
          imageInfo.components[(uint8_t)cbIndex];

        const DevEBox_JPEG::Component &crComponent =
          imageInfo.components[(uint8_t)crIndex];

        const uint8_t cbSampleX = (uint8_t)(
          ((uint16_t)x * cbComponent.horizontalSampling) / maximumH
        );

        const uint8_t cbSampleY = (uint8_t)(
          ((uint16_t)y * cbComponent.verticalSampling) / maximumV
        );

        const uint8_t crSampleX = (uint8_t)(
          ((uint16_t)x * crComponent.horizontalSampling) / maximumH
        );

        const uint8_t crSampleY = (uint8_t)(
          ((uint16_t)y * crComponent.verticalSampling) / maximumV
        );

        uint8_t cbSample;
        uint8_t crSample;

        if (!getComponentSample(
              mcu, blockMap, (uint8_t)cbIndex,
              cbComponent.horizontalSampling,
              cbSampleX, cbSampleY, cbSample
            ) ||
            !getComponentSample(
              mcu, blockMap, (uint8_t)crIndex,
              crComponent.horizontalSampling,
              crSampleX, crSampleY, crSample
            )) {
          return false;
        }

        rgb565[(uint16_t)y * width + x] =
          ycbcrToRGB565(ySample, cbSample, crSample);
      }
    }
  }

  outputWidth = width;
  outputHeight = height;
  return true;
}
