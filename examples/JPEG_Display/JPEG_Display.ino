#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include "DevEBox_ILI9341.h"
#include "DevEBox_JPEG.h"
#include "DevEBox_JPEG_Color.h"

#define SD_CS PA8
#define LCD_BACKLIGHT PA12
#define TFT_WIDTH 320
#define TFT_HEIGHT 240

const char IMAGE_PATH[] = "/IMAGE.JPG";

DevEBox_ILI9341 tft;
DevEBox_JPEG jpeg;
DevEBox_JPEG::MCU mcu;

uint16_t rgb565[DevEBox_JPEG_Color::MAX_MCU_PIXELS];

void initializeSPI() {
  SPI.setMOSI(PB5);
  SPI.setMISO(PB4);
  SPI.setSCLK(PB3);

  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_AFIO_REMAP_SWJ_NOJTAG();

  SPI.begin();
}

bool initializeSD() {
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  Serial1.println("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial1.println("SD INITIALIZATION FAILED!");
    return false;
  }

  Serial1.println("SD READY!");
  return true;
}

bool displayMCU(
  const uint16_t *pixels,
  uint8_t width,
  uint8_t height,
  uint16_t displayX,
  uint16_t displayY,
  uint16_t imageWidth,
  uint16_t imageHeight
) {
  if (pixels == nullptr || width == 0 || height == 0) return false;

  if (displayX >= imageWidth || displayY >= imageHeight ||
      displayX >= TFT_WIDTH || displayY >= TFT_HEIGHT) {
    return true;
  }

  uint16_t visibleWidth = width;
  uint16_t visibleHeight = height;

  if (displayX + visibleWidth > imageWidth)
    visibleWidth = imageWidth - displayX;
  if (displayY + visibleHeight > imageHeight)
    visibleHeight = imageHeight - displayY;
  if (displayX + visibleWidth > TFT_WIDTH)
    visibleWidth = TFT_WIDTH - displayX;
  if (displayY + visibleHeight > TFT_HEIGHT)
    visibleHeight = TFT_HEIGHT - displayY;

  for (uint16_t row = 0; row < visibleHeight; row++) {
    tft.writePixels(
      displayX,
      displayY + row,
      &pixels[row * width],
      visibleWidth
    );
  }

  return true;
}

void displayJPEG(const char *path) {
  File image = SD.open(path, FILE_READ);
  if (!image) {
    Serial1.print("ERROR: Cannot open ");
    Serial1.println(path);
    return;
  }

  Serial1.print("Opened: ");
  Serial1.println(path);
  Serial1.println("Parsing JPEG header...");

  if (!jpeg.parse(image, &Serial1)) {
    Serial1.println("JPEG HEADER PARSING FAILED!");
    image.close();
    return;
  }

  const DevEBox_JPEG::ImageInfo &info = jpeg.info();

  Serial1.print("Width: ");
  Serial1.println(info.width);
  Serial1.print("Height: ");
  Serial1.println(info.height);
  Serial1.print("Components: ");
  Serial1.println(info.componentCount);
  Serial1.print("MCU columns: ");
  Serial1.println(jpeg.mcuColumns());
  Serial1.print("MCU rows: ");
  Serial1.println(jpeg.mcuRows());

  if (info.restartIntervalDefined && info.restartInterval != 0) {
    Serial1.println("UNSUPPORTED: JPEG RESTART INTERVAL!");
    image.close();
    return;
  }

  const uint16_t columns = jpeg.mcuColumns();
  const uint16_t rows = jpeg.mcuRows();
  const uint16_t mcuWidth = jpeg.mcuWidth();
  const uint16_t mcuHeight = jpeg.mcuHeight();

  if (columns == 0 || rows == 0 || mcuWidth == 0 || mcuHeight == 0) {
    Serial1.println("ERROR: INVALID MCU GEOMETRY!");
    image.close();
    return;
  }

  jpeg.beginEntropy();
  tft.fillScreen(0x0000);

  uint32_t decodedMCUs = 0;

  for (uint16_t mcuY = 0; mcuY < rows; mcuY++) {
    Serial1.print("MCU ROW ");
    Serial1.print(mcuY + 1);
    Serial1.print("/");
    Serial1.println(rows);

    for (uint16_t mcuX = 0; mcuX < columns; mcuX++) {
      if (!jpeg.decodeMCU(image, mcu)) {
        Serial1.print("MCU DECODING FAILED AT X=");
        Serial1.print(mcuX);
        Serial1.print(", Y=");
        Serial1.println(mcuY);
        image.close();
        return;
      }

      uint8_t outputWidth = 0;
      uint8_t outputHeight = 0;

      if (!DevEBox_JPEG_Color::convertMCU(
            info, mcu, rgb565,
            DevEBox_JPEG_Color::MAX_MCU_PIXELS,
            outputWidth, outputHeight)) {
        Serial1.println("COLOR CONVERSION FAILED!");
        image.close();
        return;
      }

      if (outputWidth != mcuWidth || outputHeight != mcuHeight) {
        Serial1.println("MCU DIMENSION MISMATCH!");
        image.close();
        return;
      }

      const uint16_t displayX = mcuX * mcuWidth;
      const uint16_t displayY = mcuY * mcuHeight;

      if (!displayMCU(
            rgb565, outputWidth, outputHeight,
            displayX, displayY, info.width, info.height)) {
        Serial1.println("TFT DISPLAY FAILED!");
        image.close();
        return;
      }

      decodedMCUs++;
      Serial1.print(".");
    }

    Serial1.println(" ROW COMPLETE");
  }

  image.close();

  Serial1.println();
  Serial1.println("JPEG DECODE COMPLETE!");
  Serial1.print("Decoded MCUs: ");
  Serial1.println(decodedMCUs);
  Serial1.print("Expected MCUs: ");
  Serial1.println((uint32_t)columns * rows);
}

void setup() {
  Serial1.begin(115200);
  delay(1000);

  Serial1.println();
  Serial1.println("DevEBox JPEG Display Example");

  pinMode(LCD_BACKLIGHT, OUTPUT);
  digitalWrite(LCD_BACKLIGHT, LOW);

  tft.begin();
  tft.setBacklight(true);
  tft.fillScreen(0x0000);

  initializeSPI();

  if (!initializeSD()) return;

  displayJPEG(IMAGE_PATH);
}

void loop() {
  // Image is displayed once during setup.
}
