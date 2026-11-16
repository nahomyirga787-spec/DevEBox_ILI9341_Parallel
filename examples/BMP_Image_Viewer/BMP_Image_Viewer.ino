
#include <Arduino.h>
#include <SPI.h>
#include <SdFat.h>

#include "DevEBox_ILI9341.h"
#include "DevEBox_BMP.h"
#include "DevEBox_SD_Browser.h"

#define SD_CS PA8
#define LCD_BACKLIGHT PA12

const char IMAGE_FILENAME[] = "IMAGE.BMP";

SdFat32 sd;
DevEBox_ILI9341 tft;
DevEBox_SD_Browser browser(sd);

void initializeSPI() {
  SPI.setMOSI(PB5);
  SPI.setMISO(PB4);
  SPI.setSCLK(PB3);
  SPI.begin();

  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_AFIO_REMAP_SWJ_NOJTAG();

  Serial1.println("SPI initialized.");
}

bool initializeSD() {
  Serial1.println("Initializing SD card...");

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  if (!sd.begin(SdSpiConfig(SD_CS, SHARED_SPI, SD_SCK_MHZ(4)))) {
    Serial1.println("SD INITIALIZATION FAILED!");
    sd.printSdError(&Serial1);
    return false;
  }

  Serial1.println("SD READY!");

  if (!browser.begin()) {
    Serial1.println("FILE BROWSER INITIALIZATION FAILED!");
    return false;
  }

  Serial1.println("FILE BROWSER READY!");
  return true;
}

void setup() {
  Serial1.begin(115200);
  delay(1000);

  Serial1.println();
  Serial1.println("DevEBox BMP Image Viewer");

  pinMode(LCD_BACKLIGHT, OUTPUT);
  digitalWrite(LCD_BACKLIGHT, LOW);

  tft.begin();
  tft.setBacklight(true);
  tft.fillScreen(0x0000);

  initializeSPI();

  if (!initializeSD()) {
    Serial1.println("Check SD wiring and restart.");
    return;
  }

  File32 image = browser.openFile(IMAGE_FILENAME, Serial1);

  if (!image) {
    Serial1.println("Could not open BMP image.");
    return;
  }

  Serial1.print("File size: ");
  Serial1.println(image.fileSize());

  // Use the scaled overload at 1.0 to retain the
  // renderer's scaling and positioning behavior.
  const bool success = DevEBox_BMP::draw(
    tft,
    image,
    1.0f,
    &Serial1
  );

  image.close();

  Serial1.println(
    success ? "BMP RENDERING COMPLETE!" :
              "BMP RENDERING FAILED!"
  );
}

void loop() {
}