
#include <Arduino.h>
#include <SPI.h>
#include <SdFat.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "DevEBox_ILI9341.h"
#include "DevEBox_BMP.h"
#include "DevEBox_SD_Browser.h"
#include "DevEBox_JPEG_Display.h"

// ==========================================
// HARDWARE CONFIGURATION
// ==========================================

#define SD_CS          PA8
#define LCD_BACKLIGHT  PA12

#define SERIAL_BAUD    115200

// ==========================================
// OBJECTS
// ==========================================

SdFat32 sd;
DevEBox_ILI9341 tft;
DevEBox_SD_Browser browser(sd);

// ==========================================
// STATE
// ==========================================

static bool sdReady = false;

static float currentScale = 1.0f;

static char lastOpenedFilename[
  DevEBox_SD_Browser::MAX_FILENAME_LENGTH
] = "";

static bool hasOpenImage = false;

// Serial command buffer
static const uint8_t COMMAND_BUFFER_SIZE = 96;
static char commandBuffer[COMMAND_BUFFER_SIZE];
static uint8_t commandLength = 0;

// ==========================================
// FUNCTION DECLARATIONS
// ==========================================

void initializeSPI();
bool initializeSD();
bool initializeDisplay();

void printHelp();
void processSerialInput();
void processCommand(char *command);

bool displayImage(const char *filename);
bool redrawLastImage();

bool isBMP(const char *filename);
bool isJPEG(const char *filename);

void handleScaleCommand(const char *argument);
void clearLastImage();

// ==========================================
// SPI INITIALIZATION
// ==========================================

void initializeSPI() {

  SPI.setMOSI(PB5);
  SPI.setMISO(PB4);
  SPI.setSCLK(PB3);

  SPI.begin();

  // Release JTAG pins for SPI use.
  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_AFIO_REMAP_SWJ_NOJTAG();
}

// ==========================================
// SD INITIALIZATION
// ==========================================

bool initializeSD() {

  Serial1.println();
  Serial1.println("Initializing SD card...");

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  if (!sd.begin(
        SdSpiConfig(
          SD_CS,
          SHARED_SPI,
          SD_SCK_MHZ(4)
        )
      )) {

    Serial1.println("SD INITIALIZATION FAILED!");
    sd.printSdError(&Serial1);

    return false;
  }

  Serial1.println("SD READY!");

  return true;
}

// ==========================================
// DISPLAY INITIALIZATION
// ==========================================

bool initializeDisplay() {

  Serial1.println("Initializing TFT...");

  pinMode(LCD_BACKLIGHT, OUTPUT);
  digitalWrite(LCD_BACKLIGHT, LOW);

  tft.begin();

  tft.setBacklight(true);

  tft.fillScreen(0x0000);

  Serial1.println("TFT READY!");

  // Do not change the display rotation.
  // The tested default orientation is retained.

  return true;
}

// ==========================================
// IMAGE FORMAT DETECTION
// ==========================================

bool isBMP(const char *filename) {

  const char *extension = strrchr(filename, '.');

  if (extension == nullptr) {
    return false;
  }

  return (
    strcasecmp(extension, ".bmp") == 0
  );
}

bool isJPEG(const char *filename) {

  const char *extension = strrchr(filename, '.');

  if (extension == nullptr) {
    return false;
  }

  return (
    strcasecmp(extension, ".jpg") == 0 ||
    strcasecmp(extension, ".jpeg") == 0
  );
}

// ==========================================
// CLEAR IMAGE STATE
// ==========================================

void clearLastImage() {

  lastOpenedFilename[0] = '\0';

  hasOpenImage = false;
}

// ==========================================
// DISPLAY IMAGE
// ==========================================

bool displayImage(const char *filename) {

  if (!sdReady) {
    Serial1.println("SD CARD NOT READY!");
    return false;
  }

  if (!isBMP(filename) && !isJPEG(filename)) {

    Serial1.println("UNSUPPORTED IMAGE FORMAT!");
    Serial1.println("Supported formats: BMP, JPG, JPEG");

    return false;
  }

  Serial1.println();
  Serial1.println("================================");
  Serial1.print("Opening: ");
  Serial1.println(filename);

  File32 image = browser.openFile(filename, Serial1);

  if (!image) {

    Serial1.println("FILE OPEN FAILED!");

    return false;
  }

  Serial1.print("File size: ");
  Serial1.println(image.fileSize());

  // Remember the filename so scale changes
  // can redraw the currently displayed image.

  strncpy(
    lastOpenedFilename,
    filename,
    sizeof(lastOpenedFilename) - 1
  );

  lastOpenedFilename[
    sizeof(lastOpenedFilename) - 1
  ] = '\0';

  hasOpenImage = true;

  tft.fillScreen(0x0000);

  bool success = false;

  // ----------------------------------------
  // BMP
  // ----------------------------------------

  if (isBMP(filename)) {

    Serial1.println("Detected format: BMP");

    Serial1.print("Rendering scale: ");
    Serial1.println(currentScale, 2);

    success = DevEBox_BMP::draw(
      tft,
      image,
      currentScale,
      &Serial1
    );
  }

  // ----------------------------------------
  // JPEG
  // ----------------------------------------

  else if (isJPEG(filename)) {

    Serial1.println("Detected format: JPEG");

    Serial1.print("Rendering scale: ");
    Serial1.println(currentScale, 2);

    success = DevEBox_JPEG_Display::draw(
      tft,
      image,
      currentScale,
      &Serial1
    );
  }

  image.close();

  Serial1.println("--------------------------------");

  if (success) {
    Serial1.println("IMAGE DISPLAYED SUCCESSFULLY!");
  }
  else {
    Serial1.println("IMAGE RENDERING FAILED!");
  }

  Serial1.println("================================");

  return success;
}

// ==========================================
// REDRAW CURRENT IMAGE
// ==========================================

bool redrawLastImage() {

  if (!hasOpenImage || lastOpenedFilename[0] == '\0') {

    Serial1.println("NO IMAGE TO REDRAW.");

    return false;
  }

  Serial1.println();
  Serial1.println("Redrawing current image...");

  return displayImage(lastOpenedFilename);
}

// ==========================================
// SCALE COMMAND
// ==========================================

void handleScaleCommand(const char *argument) {

  // No argument: display current scale.

  if (argument == nullptr || argument[0] == '\0') {

    Serial1.print("Current scale: ");
    Serial1.print(currentScale, 2);
    Serial1.println("x");

    return;
  }

  char *endPointer = nullptr;

  float requestedScale = strtof(
    argument,
    &endPointer
  );

  // Check whether a valid number was supplied.

  if (
    endPointer == argument ||
    !isfinite(requestedScale) ||
    requestedScale < 0.1f ||
    requestedScale > 4.0f
  ) {

    Serial1.println("INVALID SCALE!");
    Serial1.println("Allowed range: 0.1 to 4.0");

    return;
  }

  // Reject trailing non-whitespace characters.

  while (*endPointer == ' ' || *endPointer == '\t') {
    endPointer++;
  }

  if (*endPointer != '\0') {

    Serial1.println("INVALID SCALE FORMAT!");

    return;
  }

  currentScale = requestedScale;

  Serial1.print("Scale updated to: ");
  Serial1.print(currentScale, 2);
  Serial1.println("x");

  // Automatically redraw the currently displayed image.

  if (hasOpenImage) {

    redrawLastImage();
  }
  else {

    Serial1.println("Open an image to apply the new scale.");
  }
}

// ==========================================
// HELP
// ==========================================

void printHelp() {

  Serial1.println();
  Serial1.println("================================");
  Serial1.println("       DevEBox IMAGE BROWSER");
  Serial1.println("================================");

  Serial1.println("ls                 List directory");
  Serial1.println("cd FOLDER          Enter directory");
  Serial1.println("cd /               Return to root");
  Serial1.println("up                 Parent directory");
  Serial1.println("pwd                Show current path");

  Serial1.println("open FILE.BMP      Open BMP image");
  Serial1.println("open FILE.JPG      Open JPEG image");

  Serial1.println("scale              Show current scale");
  Serial1.println("scale 0.5          Set scale to 0.5x");
  Serial1.println("scale 1.0          Set scale to 1.0x");
  Serial1.println("scale 2.0          Set scale to 2.0x");

  Serial1.println("help               Show commands");

  Serial1.println("--------------------------------");
  Serial1.println("Supported image formats:");
  Serial1.println("BMP, JPG, JPEG");

  Serial1.println("--------------------------------");
  Serial1.println("Scale range: 0.1x - 4.0x");
  Serial1.println("Scaling applies to BMP and JPEG.");
  Serial1.println("Changing scale redraws the image.");
  Serial1.println("================================");
  Serial1.println();
}

// ==========================================
// COMMAND PROCESSING
// ==========================================

void processCommand(char *command) {

  // Remove leading whitespace.

  while (*command == ' ' || *command == '\t') {
    command++;
  }

  // Remove trailing whitespace.

  size_t length = strlen(command);

  while (
    length > 0 &&
    (
      command[length - 1] == ' ' ||
      command[length - 1] == '\t' ||
      command[length - 1] == '\r'
    )
  ) {

    command[length - 1] = '\0';
    length--;
  }

  if (length == 0) {
    return;
  }

  // ----------------------------------------
  // HELP
  // ----------------------------------------

  if (strcasecmp(command, "help") == 0) {

    printHelp();
    return;
  }

  // ----------------------------------------
  // LIST DIRECTORY
  // ----------------------------------------

  if (strcasecmp(command, "ls") == 0) {

    browser.listDirectory(Serial1);
    return;
  }

  // ----------------------------------------
  // PRINT CURRENT DIRECTORY
  // ----------------------------------------

  if (strcasecmp(command, "pwd") == 0) {

    Serial1.print("Current path: ");
    Serial1.println(browser.currentPath());

    return;
  }

  // ----------------------------------------
  // PARENT DIRECTORY
  // ----------------------------------------

  if (strcasecmp(command, "up") == 0) {

    if (browser.goUp(Serial1)) {
      clearLastImage();
    }

    return;
  }

  // ----------------------------------------
  // CHANGE DIRECTORY
  // ----------------------------------------

  if (strncasecmp(command, "cd ", 3) == 0) {

    const char *directory = command + 3;

    while (*directory == ' ' || *directory == '\t') {
      directory++;
    }

    if (*directory == '\0') {
      Serial1.println("DIRECTORY NAME REQUIRED!");
      return;
    }

    if (browser.changeDirectory(directory, Serial1)) {
      clearLastImage();
    }

    return;
  }

  // ----------------------------------------
  // OPEN IMAGE
  // ----------------------------------------

  if (strncasecmp(command, "open ", 5) == 0) {

    const char *filename = command + 5;

    while (*filename == ' ' || *filename == '\t') {
      filename++;
    }

    if (*filename == '\0') {
      Serial1.println("FILENAME REQUIRED!");
      return;
    }

    displayImage(filename);

    return;
  }

  // ----------------------------------------
  // SCALE
  // ----------------------------------------

  if (
    strcasecmp(command, "scale") == 0 ||
    strncasecmp(command, "scale ", 6) == 0
  ) {

    const char *argument = command + 5;

    while (*argument == ' ' || *argument == '\t') {
      argument++;
    }

    handleScaleCommand(argument);

    return;
  }

  // ----------------------------------------
  // UNKNOWN COMMAND
  // ----------------------------------------

  Serial1.print("UNKNOWN COMMAND: ");
  Serial1.println(command);

  Serial1.println("Type 'help' for available commands.");
}

// ==========================================
// SERIAL INPUT
// ==========================================

void processSerialInput() {

  while (Serial1.available()) {

    char incoming = (char)Serial1.read();

    // End of command.

    if (incoming == '\n' || incoming == '\r') {

      if (commandLength > 0) {

        commandBuffer[commandLength] = '\0';

        processCommand(commandBuffer);

        commandLength = 0;
      }

      continue;
    }

    // Store incoming character.

    if (commandLength < COMMAND_BUFFER_SIZE - 1) {

      commandBuffer[commandLength++] = incoming;
    }
    else {

      // Prevent processing a truncated command.

      commandLength = 0;

      Serial1.println();
      Serial1.println("COMMAND TOO LONG!");
    }
  }
}

// ==========================================
// SETUP
// ==========================================

void setup() {

  Serial1.begin(SERIAL_BAUD);

  delay(1000);

  Serial1.println();
  Serial1.println("================================");
  Serial1.println(" DevEBox Universal Image Browser");
  Serial1.println(" STM32F103C8T6 Blue Pill");
  Serial1.println("================================");

  // ----------------------------------------
  // DISPLAY
  // ----------------------------------------

  initializeDisplay();

  // ----------------------------------------
  // SPI AND SD
  // ----------------------------------------

  initializeSPI();

  sdReady = initializeSD();

  if (!sdReady) {

    Serial1.println("SYSTEM STARTUP FAILED.");
    return;
  }

  // ----------------------------------------
  // FILE BROWSER
  // ----------------------------------------

  if (!browser.begin()) {

    Serial1.println("FILE BROWSER INITIALIZATION FAILED!");

    sdReady = false;

    return;
  }

  Serial1.println();
  Serial1.println("FILE BROWSER READY!");

  currentScale = 1.0f;

  clearLastImage();

  printHelp();

  browser.listDirectory(Serial1);

  Serial1.println();
  Serial1.println("READY. ENTER A COMMAND:");
}

// ==========================================
// MAIN LOOP
// ==========================================

void loop() {

  if (!sdReady) {
    return;
  }

  processSerialInput();
}