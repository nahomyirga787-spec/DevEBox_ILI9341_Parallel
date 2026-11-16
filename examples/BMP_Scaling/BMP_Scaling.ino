
#include <Arduino.h>
#include <SPI.h>
#include <SdFat.h>

#include "DevEBox_ILI9341.h"
#include "DevEBox_BMP.h"
#include "DevEBox_SD_Browser.h"

#define SD_CS PA8
#define LCD_BACKLIGHT PA12

const char IMAGE_FILENAME[] = "IMAGE.BMP";
const float IMAGE_SCALE = 1.0f;

SdFat32 sd;
DevEBox_ILI9341 tft;
DevEBox_SD_Browser browser(sd);

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

  SPI.setMOSI(PB5);
  SPI.setMISO(PB4);
  SPI.setSCLK(PB3);
  SPI.begin();

  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_AFIO_REMAP_SWJ_NOJTAG();

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  if (!sd.begin(SdSpiConfig(SD_CS, SHARED_SPI, SD_SCK_MHZ(4)))) {
    Serial1.println("SD INITIALIZATION FAILED!");
    sd.printSdError(&Serial1);
    return;
  }

  Serial1.println("SD READY!");

  if (!browser.begin()) {
    Serial1.println("FILE BROWSER INITIALIZATION FAILED!");
    return;
  }

  File32 image = browser.openFile(IMAGE_FILENAME, Serial1);

  if (!image) {
    Serial1.println("Could not open BMP image.");
    return;
  }

  Serial1.print("Scale: ");
  Serial1.println(IMAGE_SCALE, 2);

  const bool success = DevEBox_BMP::draw(
    tft,
    image,
    IMAGE_SCALE,
    &Serial1
  );

  image.close();

  Serial1.println(
    success ? "BMP SCALING COMPLETE!" :
              "BMP SCALING FAILED!"
  );
}

void loop() {
}