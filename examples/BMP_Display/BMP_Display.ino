#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include "DevEBox_ILI9341.h"
#include "DevEBox_BMP.h"

#define SD_CS PA8
#define LCD_BACKLIGHT PA12

// Change this path to a BMP file stored on the SD card.
const char IMAGE_PATH[] = "/IMAGE.BMP";

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

void displayBMP(const char *path) {
  File image = SD.open(path, FILE_READ);

  if (!image) {
    Serial1.print("ERROR: Cannot open ");
    Serial1.println(path);
    return;
  }

  Serial1.print("Opened: ");
  Serial1.println(path);
  Serial1.print("File size: ");
  Serial1.println(image.size());

  tft.fillScreen(0x0000);

  const bool success = DevEBox_BMP::draw(
    tft,
    image,
    1.0f,
    &Serial1
  );

  image.close();

  Serial1.println(success ? "BMP DISPLAY COMPLETE!" : "BMP DISPLAY FAILED!");
}

void setup() {
  Serial1.begin(115200);
  delay(1000);

  Serial1.println();
  Serial1.println("DevEBox BMP Display Example");

  pinMode(LCD_BACKLIGHT, OUTPUT);
  digitalWrite(LCD_BACKLIGHT, LOW);

  tft.begin();
  tft.setBacklight(true);
  tft.fillScreen(0x0000);
  Serial1.println("TFT READY!");

  initializeSPI();

  if (!initializeSD()) {
    return;
  }

  displayBMP(IMAGE_PATH);
}

void loop() {
  // Image is displayed once during setup.
}
