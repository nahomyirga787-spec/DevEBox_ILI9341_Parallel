#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include "DevEBox_ILI9341.h"
#include "DevEBox_BMP.h"

#define SD_CS PA8
#define LCD_BACKLIGHT PA12

const char IMAGE_PATH[] = "/IMAGE.BMP";

// Change this value and upload again to try another scale.
// Examples: 1.0f, 0.75f, 0.5f, 0.25f.
const float IMAGE_SCALE = 1.0f;

DevEBox_ILI9341 tft;

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

void displayScaledBMP(const char *path, float scale) {
  if (scale <= 0.0f) {
    Serial1.println("ERROR: Scale must be greater than zero.");
    return;
  }

  File image = SD.open(path, FILE_READ);
  if (!image) {
    Serial1.print("ERROR: Cannot open ");
    Serial1.println(path);
    return;
  }

  Serial1.print("Image: ");
  Serial1.println(path);
  Serial1.print("Scale: ");
  Serial1.println(scale, 2);

  tft.fillScreen(0x0000);

  const bool success = DevEBox_BMP::draw(
    tft,
    image,
    scale,
    &Serial1
  );

  image.close();

  Serial1.println(success ? "SCALED BMP DISPLAY COMPLETE!" :
                            "SCALED BMP DISPLAY FAILED!");
}

void setup() {
  Serial1.begin(115200);
  delay(1000);

  Serial1.println();
  Serial1.println("DevEBox BMP Scaling Example");

  pinMode(LCD_BACKLIGHT, OUTPUT);
  digitalWrite(LCD_BACKLIGHT, LOW);

  tft.begin();
  tft.setBacklight(true);
  tft.fillScreen(0x0000);

  initializeSPI();

  if (!initializeSD()) {
    return;
  }

  displayScaledBMP(IMAGE_PATH, IMAGE_SCALE);
}

void loop() {
  // Image is displayed once during setup.
}
