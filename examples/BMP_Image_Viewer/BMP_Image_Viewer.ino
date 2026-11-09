#include <SPI.h>
#include <SD.h>
#include <DevEBox_ILI9341.h>
#include <DevEBox_BMP.h>

// Blue Pill SPI1 remap:
// SCK  PB3, MISO PB4, MOSI PB5
#define SD_CS PA8

DevEBox_ILI9341 tft;

void setup() {
  Serial1.begin(115200);
  delay(1000);

  // PB3/PB4/PB5 are SPI1 remapped pins.
  // Disable JTAG but retain SWD.
  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_AFIO_REMAP_SWJ_NOJTAG();

  SPI.setMOSI(PB5);
  SPI.setMISO(PB4);
  SPI.setSCLK(PB3);

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  SPI.begin();

  tft.begin();
  tft.fillScreen(0x0000);

  bool sdReady = false;
  for (uint8_t attempt = 1; attempt <= 5; ++attempt) {
    Serial1.print("SD initialization attempt ");
    Serial1.println(attempt);

    if (SD.begin(SD_CS)) {
      sdReady = true;
      break;
    }
    delay(500);
  }

  if (!sdReady) {
    Serial1.println("SD CARD INITIALIZATION FAILED!");
    tft.setTextColor(0xF800);
    tft.setTextSize(2);
    tft.setCursor(20, 100);
    tft.println("SD CARD ERROR");
    return;
  }

  Serial1.println("SD CARD INITIALIZED SUCCESSFULLY!");

  File image = SD.open("/IMAGE.BMP", FILE_READ);
  if (!image) {
    Serial1.println("IMAGE.BMP NOT FOUND!");
    return;
  }

  const bool success = DevEBox_BMP::draw(tft, image, &Serial1);
  image.close();

  if (!success) {
    Serial1.println("IMAGE DISPLAY FAILED!");
  }
}

void loop() {
}
