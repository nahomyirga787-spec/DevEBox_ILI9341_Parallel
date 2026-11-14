#include "DevEBox_JPEG_Display.h"

uint32_t DevEBox_JPEG_Display::ceilDivide(
  uint64_t numerator,
  uint32_t denominator
) {
  return (uint32_t)((numerator + denominator - 1U) / denominator);
}


uint16_t DevEBox_JPEG_Display::mapOutputToSource(
  uint32_t outputCoordinate,
  uint16_t sourceLength,
  uint32_t outputLength
) {
  return (uint16_t)((uint64_t)outputCoordinate * sourceLength / outputLength);
}


bool DevEBox_JPEG_Display::renderMCU(
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
) {
  if (pixels == nullptr || sourceMCUWidth == 0 || sourceMCUHeight == 0 ||
      sourceImageWidth == 0 || sourceImageHeight == 0 ||
      outputImageWidth == 0 || outputImageHeight == 0) {
    return false;
  }

  const uint32_t sourceStartX = (uint32_t)mcuX * sourceMCUWidth;
  const uint32_t sourceStartY = (uint32_t)mcuY * sourceMCUHeight;

  if (sourceStartX >= sourceImageWidth || sourceStartY >= sourceImageHeight) {
    return true;
  }

  uint32_t sourceEndX = sourceStartX + sourceMCUWidth;
  uint32_t sourceEndY = sourceStartY + sourceMCUHeight;
  if (sourceEndX > sourceImageWidth) sourceEndX = sourceImageWidth;
  if (sourceEndY > sourceImageHeight) sourceEndY = sourceImageHeight;

  const uint32_t outputStartX = ceilDivide(
    (uint64_t)sourceStartX * outputImageWidth, sourceImageWidth
  );
  const uint32_t outputEndX = ceilDivide(
    (uint64_t)sourceEndX * outputImageWidth, sourceImageWidth
  );
  const uint32_t outputStartY = ceilDivide(
    (uint64_t)sourceStartY * outputImageHeight, sourceImageHeight
  );
  const uint32_t outputEndY = ceilDivide(
    (uint64_t)sourceEndY * outputImageHeight, sourceImageHeight
  );

  const int32_t screenWidth = tft.width();
  const int32_t screenHeight = tft.height();

  int32_t visibleStartX = displayOffsetX + (int32_t)outputStartX;
  int32_t visibleEndX = displayOffsetX + (int32_t)outputEndX;
  if (visibleStartX < 0) visibleStartX = 0;
  if (visibleEndX > screenWidth) visibleEndX = screenWidth;
  if (visibleStartX >= visibleEndX) return true;

  const uint16_t segmentWidth = (uint16_t)(visibleEndX - visibleStartX);
  const uint32_t firstOutputX = (uint32_t)(visibleStartX - displayOffsetX);

  for (uint32_t outputY = outputStartY; outputY < outputEndY; outputY++) {
    const int32_t screenY = displayOffsetY + (int32_t)outputY;
    if (screenY < 0 || screenY >= screenHeight) continue;

    const uint16_t sourceY = mapOutputToSource(
      outputY, sourceImageHeight, outputImageHeight
    );
    const uint32_t localY = sourceY - sourceStartY;
    if (localY >= sourceMCUHeight) return false;

    for (uint16_t i = 0; i < segmentWidth; i++) {
      const uint32_t outputX = firstOutputX + i;
      const uint16_t sourceX = mapOutputToSource(
        outputX, sourceImageWidth, outputImageWidth
      );
      const uint32_t localX = sourceX - sourceStartX;
      if (localX >= sourceMCUWidth) return false;

      lineBuffer_[i] = pixels[localY * sourceMCUWidth + localX];
    }

    tft.writePixels(
      (uint16_t)visibleStartX,
      (uint16_t)screenY,
      lineBuffer_,
      segmentWidth
    );
  }

  return true;
}


bool DevEBox_JPEG_Display::draw(
  DevEBox_ILI9341 &tft,
  File &file,
  float scale,
  Stream *diagnostics
) {
  // Keep the sizeable decoder and MCU buffers in static storage rather
  // than placing them on the caller's stack. This also mirrors the simple
  // static draw() interface used by the BMP renderer.
  static DevEBox_JPEG_Display renderer;
  return renderer.drawInternal(tft, file, scale, diagnostics);
}


bool DevEBox_JPEG_Display::drawInternal(
  DevEBox_ILI9341 &tft,
  File &file,
  float scale,
  Stream *diagnostics
) {
  if (!file || scale <= 0.0f || scale > 16.0f) {
    if (diagnostics) diagnostics->println("JPEG DISPLAY ERROR: INVALID FILE OR SCALE.");
    return false;
  }

  if (!decoder_.parse(file, diagnostics)) {
    if (diagnostics) diagnostics->println("JPEG HEADER PARSING FAILED!");
    return false;
  }

  const DevEBox_JPEG::ImageInfo &info = decoder_.info();
  const uint16_t columns = decoder_.mcuColumns();
  const uint16_t rows = decoder_.mcuRows();
  const uint16_t mcuWidth = decoder_.mcuWidth();
  const uint16_t mcuHeight = decoder_.mcuHeight();

  if (columns == 0 || rows == 0 || mcuWidth == 0 || mcuHeight == 0) {
    if (diagnostics) diagnostics->println("JPEG DISPLAY ERROR: INVALID MCU GEOMETRY.");
    return false;
  }

  uint32_t outputWidth = (uint32_t)((float)info.width * scale + 0.5f);
  uint32_t outputHeight = (uint32_t)((float)info.height * scale + 0.5f);
  if (outputWidth == 0) outputWidth = 1;
  if (outputHeight == 0) outputHeight = 1;

  const int32_t offsetX = ((int32_t)tft.width() - (int32_t)outputWidth) / 2;
  const int32_t offsetY = ((int32_t)tft.height() - (int32_t)outputHeight) / 2;

  if (diagnostics) {
    diagnostics->println("JPEG IMAGE INFORMATION");
    diagnostics->print("Source size: ");
    diagnostics->print(info.width);
    diagnostics->print(" x ");
    diagnostics->println(info.height);
    diagnostics->print("Scale: ");
    diagnostics->println(scale, 3);
    diagnostics->print("Rendered size: ");
    diagnostics->print(outputWidth);
    diagnostics->print(" x ");
    diagnostics->println(outputHeight);
    diagnostics->print("Restart interval: ");
    diagnostics->println(info.restartIntervalDefined ? info.restartInterval : 0);
  }

  decoder_.beginEntropy();
  tft.fillScreen(0x0000);

  uint32_t decodedMCUs = 0;
  for (uint16_t mcuY = 0; mcuY < rows; mcuY++) {
    for (uint16_t mcuX = 0; mcuX < columns; mcuX++) {
      if (!decoder_.decodeMCU(file, mcu_)) {
        if (diagnostics) {
          diagnostics->print("JPEG MCU DECODING FAILED AT X=");
          diagnostics->print(mcuX);
          diagnostics->print(", Y=");
          diagnostics->println(mcuY);
        }
        return false;
      }

      uint8_t convertedWidth = 0;
      uint8_t convertedHeight = 0;
      if (!DevEBox_JPEG_Color::convertMCU(
            info, mcu_, mcuPixels_,
            DevEBox_JPEG_Color::MAX_MCU_PIXELS,
            convertedWidth, convertedHeight)) {
        if (diagnostics) diagnostics->println("JPEG COLOR CONVERSION FAILED!");
        return false;
      }

      if (convertedWidth != mcuWidth || convertedHeight != mcuHeight) {
        if (diagnostics) diagnostics->println("JPEG MCU DIMENSION MISMATCH!");
        return false;
      }

      if (!renderMCU(
            tft, mcuPixels_, convertedWidth, convertedHeight,
            mcuX, mcuY, info.width, info.height,
            outputWidth, outputHeight, offsetX, offsetY)) {
        if (diagnostics) diagnostics->println("JPEG TFT RENDERING FAILED!");
        return false;
      }

      decodedMCUs++;
    }
  }

  if (diagnostics) {
    diagnostics->println("JPEG RENDERING COMPLETE!");
    diagnostics->print("Decoded MCUs: ");
    diagnostics->println(decodedMCUs);
  }
  return true;
}
