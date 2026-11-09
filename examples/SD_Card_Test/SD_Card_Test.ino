#include <SPI.h>
#include <SD.h>

// Blue Pill SPI1 remap:
// SCK  PB3, MISO PB4, MOSI PB5
// Keep SD chip-select separate from the TFT control pins.
#define SD_CS PA8

void setup() {
  Serial1.begin(115200);
  delay(1000);

  // Disable JTAG while retaining SWD so PB3/PB4/PB5 can be used.
  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_AFIO_REMAP_SWJ_NOJTAG();

  SPI.setMOSI(PB5);
  SPI.setMISO(PB4);
  SPI.setSCLK(PB3);

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  SPI.begin();

  Serial1.println("Initializing SD card...");

  if (!SD.begin(SD_CS)) {
    Serial1.println("SD initialization failed.");
    return;
  }

  Serial1.println("SD initialized.");

  File file = SD.open("/HELLO.TXT", FILE_READ);
  if (!file) {
    Serial1.println("HELLO.TXT not found.");
    return;
  }

  Serial1.print("File size: ");
  Serial1.println(file.size());

  while (file.available()) {
    Serial1.write(file.read());
  }

  file.close();
}

void loop() {
}
