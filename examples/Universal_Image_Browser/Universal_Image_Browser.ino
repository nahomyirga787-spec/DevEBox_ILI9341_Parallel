#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <string.h>

#include "DevEBox_ILI9341.h"
#include "DevEBox_BMP.h"
#include "DevEBox_SD_Browser.h"
#include "DevEBox_JPEG.h"
#include "DevEBox_JPEG_Color.h"

#define SD_CS PA8
#define LCD_BACKLIGHT PA12
#define TFT_WIDTH 320
#define TFT_HEIGHT 240

DevEBox_ILI9341 tft;
DevEBox_SD_Browser browser;
DevEBox_JPEG jpeg;
DevEBox_JPEG::MCU mcu;

uint16_t rgb565[DevEBox_JPEG_Color::MAX_MCU_PIXELS];

bool sdReady = false;
char commandBuffer[96];
uint8_t commandIndex = 0;

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

  if (!SD.begin(SD_CS)) {
    Serial1.println("SD INITIALIZATION FAILED!");
    return false;
  }

  Serial1.println("SD READY!");
  return true;
}

char toLowerASCII(char c) {
  if (c >= 'A' && c <= 'Z') return c + ('a' - 'A');
  return c;
}

bool extensionMatches(const char *filename, const char *extension) {
  if (filename == nullptr || extension == nullptr) return false;

  const char *dot = strrchr(filename, '.');
  if (dot == nullptr) return false;

  while (*dot && *extension) {
    if (toLowerASCII(*dot) != toLowerASCII(*extension)) return false;
    dot++;
    extension++;
  }

  return (*dot == '\0' && *extension == '\0');
}

void displayBMP(const char *filename) {
  File image = browser.openFile(filename, Serial1);
  if (!image) return;

  Serial1.print("File size: ");
  Serial1.println(image.size());

  tft.fillScreen(0x0000);

  const bool success = DevEBox_BMP::draw(
    tft, image, 1.0f, &Serial1
  );

  image.close();
  Serial1.println(success ? "BMP RENDERING COMPLETE!" :
                            "BMP RENDERING FAILED!");
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
      displayX >= TFT_WIDTH || displayY >= TFT_HEIGHT) return true;

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
      displayX, displayY + row,
      &pixels[row * width], visibleWidth
    );
  }

  return true;
}

void displayJPEG(const char *filename) {
  Serial1.println();
  Serial1.println("JPEG IMAGE VIEWER");

  File image = browser.openFile(filename, Serial1);
  if (!image) return;

  Serial1.print("File size: ");
  Serial1.println(image.size());
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

void openImage(const char *filename) {
  if (extensionMatches(filename, ".bmp")) {
    displayBMP(filename);
  } else if (extensionMatches(filename, ".jpg") ||
             extensionMatches(filename, ".jpeg")) {
    displayJPEG(filename);
  } else {
    Serial1.println("ERROR: Unsupported image format.");
    Serial1.println("Supported formats: BMP, JPG, JPEG");
  }
}

void printHelp() {
  Serial1.println();
  Serial1.println("================================");
  Serial1.println("     UNIVERSAL IMAGE BROWSER");
  Serial1.println("================================");
  Serial1.println("ls                 List directory");
  Serial1.println("cd FOLDER          Enter directory");
  Serial1.println("cd /               Return to root");
  Serial1.println("up                 Parent directory");
  Serial1.println("pwd                Show current path");
  Serial1.println("open IMAGE.BMP     Display BMP");
  Serial1.println("open IMAGE.JPG     Display JPEG");
  Serial1.println("open IMAGE.JPEG    Display JPEG");
  Serial1.println("help               Show commands");
  Serial1.println("================================");
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
    if (argument == nullptr || *argument == '\0')
      Serial1.println("Usage: cd FOLDER");
    else
      browser.changeDirectory(argument, Serial1);
  } else if (strcmp(line, "up") == 0) {
    browser.goUp(Serial1);
  } else if (strcmp(line, "pwd") == 0) {
    Serial1.print("Current directory: ");
    Serial1.println(browser.currentPath());
  } else if (strcmp(line, "open") == 0) {
    if (argument == nullptr || *argument == '\0')
      Serial1.println("Usage: open IMAGE.BMP/JPG/JPEG");
    else
      openImage(argument);
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
  Serial1.println("DevEBox Universal Image Browser");
  Serial1.println("STM32F103C8T6 Blue Pill");

  pinMode(LCD_BACKLIGHT, OUTPUT);
  digitalWrite(LCD_BACKLIGHT, LOW);

  tft.begin();
  tft.setBacklight(true);
  tft.fillScreen(0x0000);
  Serial1.println("TFT READY!");

  initializeSPI();

  sdReady = initializeSD();
  if (!sdReady) {
    Serial1.println("Check SD wiring and restart.");
    return;
  }

  if (!browser.begin()) {
    Serial1.println("ERROR: Cannot access root directory.");
    sdReady = false;
    return;
  }

  Serial1.println("FILE BROWSER READY!");
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
