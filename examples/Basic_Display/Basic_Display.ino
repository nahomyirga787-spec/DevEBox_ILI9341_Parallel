
#include <Arduino.h>
#include "DevEBox_ILI9341.h"

// ============================================================
// DevEBox ILI9341 - Basic Display Example
// Board: STM32F103C8T6 Blue Pill
// Display: DevEBox 3.2-inch ILI9341, 320x240
// ============================================================

DevEBox_ILI9341 tft;

// RGB565 color definitions
#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define GREEN   0x07E0
#define BLUE    0x001F
#define YELLOW  0xFFE0
#define CYAN    0x07FF
#define MAGENTA 0xF81F

void setup() {

  Serial1.begin(115200);
  delay(1000);

  Serial1.println();
  Serial1.println("==============================");
  Serial1.println(" DevEBox Basic Display Example");
  Serial1.println("==============================");

  // Initialize the TFT.
  Serial1.println("Initializing display...");

  tft.begin();
  tft.setBacklight(true);

  Serial1.println("Display initialized!");

  // Display a sequence of RGB565 colors.
  Serial1.println("Starting color demonstration...");

  tft.fillScreen(RED);
  delay(1000);

  tft.fillScreen(GREEN);
  delay(1000);

  tft.fillScreen(BLUE);
  delay(1000);

  tft.fillScreen(YELLOW);
  delay(1000);

  tft.fillScreen(CYAN);
  delay(1000);

  tft.fillScreen(MAGENTA);
  delay(1000);

  tft.fillScreen(WHITE);
  delay(1000);

  tft.fillScreen(BLACK);

  Serial1.println("Color demonstration complete!");
}

void loop() {
  // Nothing to do.
}
