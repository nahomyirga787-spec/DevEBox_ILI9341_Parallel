#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#define SD_CS PA8

void initializeSPI() {
  SPI.setMOSI(PB5);
  SPI.setMISO(PB4);
  SPI.setSCLK(PB3);

  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_AFIO_REMAP_SWJ_NOJTAG();

  SPI.begin();
}

void listRootDirectory() {
  Serial1.println();
  Serial1.println("ROOT DIRECTORY");

  File root = SD.open("/");
  if (!root || !root.isDirectory()) {
    Serial1.println("ERROR: Cannot open root directory.");
    root.close();
    return;
  }

  File entry = root.openNextFile();
  uint16_t count = 0;

  while (entry) {
    Serial1.print(entry.isDirectory() ? "[DIR]  " : "[FILE] ");
    Serial1.print(entry.name());

    if (!entry.isDirectory()) {
      Serial1.print("  ");
      Serial1.print(entry.size());
      Serial1.print(" bytes");
    }

    Serial1.println();
    entry.close();
    count++;
    entry = root.openNextFile();
  }

  root.close();
  Serial1.print("Total entries: ");
  Serial1.println(count);
}

void readTestFile(const char *path) {
  Serial1.println();
  Serial1.print("Reading: ");
  Serial1.println(path);

  File file = SD.open(path, FILE_READ);
  if (!file) {
    Serial1.println("File not found. Add the file to the SD card to test reading.");
    return;
  }

  while (file.available()) {
    Serial1.write(file.read());
  }

  file.close();
  Serial1.println();
  Serial1.println("READ COMPLETE.");
}

void setup() {
  Serial1.begin(115200);
  delay(1000);

  Serial1.println();
  Serial1.println("DevEBox SD Card Test");
  Serial1.println("STM32F103C8T6 Blue Pill");

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  initializeSPI();

  Serial1.println("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial1.println("SD INITIALIZATION FAILED!");
    return;
  }

  Serial1.println("SD INITIALIZED SUCCESSFULLY!");

  listRootDirectory();

  // Place HELLO.TXT in the SD card root to test file reading.
  readTestFile("/HELLO.TXT");
}

void loop() {
  // Test runs once during setup.
}
