#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <string.h>

#include "DevEBox_ILI9341.h"
#include "DevEBox_BMP.h"
#include "DevEBox_SD_Browser.h"

#define SD_CS PA8
#define LCD_BACKLIGHT PA12

DevEBox_ILI9341 tft;
DevEBox_SD_Browser browser;

bool sdReady = false;
char commandBuffer[96];
uint8_t commandIndex = 0;

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

void displayBMP(const char *filename) {
  File image = browser.openFile(filename, Serial1);
  if (!image) {
    return;
  }

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

void printHelp() {
  Serial1.println();
  Serial1.println("SD FILE BROWSER COMMANDS");
  Serial1.println("ls               List directory");
  Serial1.println("cd FOLDER        Enter directory");
  Serial1.println("cd /             Return to root");
  Serial1.println("up               Parent directory");
  Serial1.println("pwd              Show current path");
  Serial1.println("open IMAGE.BMP   Display a BMP image");
  Serial1.println("help             Show commands");
}

void processCommand(char *line) {
  while (*line == ' ') line++;
  if (*line == '\0') return;

  char *argument = strchr(line, ' ');
  if (argument != nullptr) {
    *argument++ = '\0';
    while (*argument == ' ') argument++;
  }

  if (strcmp(line, "ls") == 0) {
    browser.listDirectory(Serial1);
  } else if (strcmp(line, "cd") == 0) {
    if (argument == nullptr || *argument == '\0') {
      Serial1.println("Usage: cd FOLDER");
    } else {
      browser.changeDirectory(argument, Serial1);
    }
  } else if (strcmp(line, "up") == 0) {
    browser.goUp(Serial1);
  } else if (strcmp(line, "pwd") == 0) {
    Serial1.print("Current directory: ");
    Serial1.println(browser.currentPath());
  } else if (strcmp(line, "open") == 0) {
    if (argument == nullptr || *argument == '\0') {
      Serial1.println("Usage: open IMAGE.BMP");
    } else {
      displayBMP(argument);
    }
  } else if (strcmp(line, "help") == 0) {
    printHelp();
  } else {
    Serial1.println("Unknown command. Type help.");
  }
}

void readSerialCommand() {
  while (Serial1.available()) {
    const char incoming = (char)Serial1.read();

    if (incoming == '\r') continue;

    if (incoming == '\n') {
      commandBuffer[commandIndex] = '\0';
      processCommand(commandBuffer);
      commandIndex = 0;

      Serial1.print(browser.currentPath());
      Serial1.print(" > ");
    } else if (incoming == '\b' || incoming == 127) {
      if (commandIndex > 0) {
        commandIndex--;
        Serial1.print("\b \b");
      }
    } else if (commandIndex < sizeof(commandBuffer) - 1) {
      commandBuffer[commandIndex++] = incoming;
    } else {
      Serial1.println();
      Serial1.println("ERROR: Command too long.");
      commandIndex = 0;
    }
  }
}

void setup() {
  Serial1.begin(115200);
  delay(1000);

  Serial1.println();
  Serial1.println("DevEBox SD File Browser - BMP");
  Serial1.println("STM32F103C8T6 Blue Pill");

  pinMode(LCD_BACKLIGHT, OUTPUT);
  digitalWrite(LCD_BACKLIGHT, LOW);

  tft.begin();
  tft.setBacklight(true);
  tft.fillScreen(0x0000);

  initializeSPI();

  sdReady = initializeSD();
  if (!sdReady) return;

  if (!browser.begin()) {
    Serial1.println("ERROR: Cannot access root directory.");
    sdReady = false;
    return;
  }

  printHelp();
  browser.listDirectory(Serial1);

  Serial1.print(browser.currentPath());
  Serial1.print(" > ");
}

void loop() {
  if (sdReady) {
    readSerialCommand();
  }
}
