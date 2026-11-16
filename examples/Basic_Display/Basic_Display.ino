#include <DevEBox_ILI9341.h>

DevEBox_ILI9341 tft;

// RGB565 colors
#define COLOR_BLACK       0x0000
#define COLOR_WHITE       0xFFFF
#define COLOR_RED         0xF800
#define COLOR_GREEN       0x07E0
#define COLOR_BLUE        0x001F
#define COLOR_YELLOW      0xFFE0
#define COLOR_CYAN        0x07FF
#define COLOR_MAGENTA     0xF81F
#define COLOR_ORANGE      0xFD20

void drawDemoScreen();

void setup() {
  tft.begin();
  tft.setBacklight(true);
  tft.setRotation(1);

  drawDemoScreen();
}

void loop() {
  // Nothing to do.
}

void drawDemoScreen() {
  const int16_t screenW = tft.width();

  tft.fillScreen(COLOR_BLACK);

  // Header
  tft.fillRect(0, 0, screenW, 38, COLOR_BLUE);
  tft.setTextSize(2);
  tft.setTextColor(COLOR_WHITE);
  tft.setCursor(10, 11);
  tft.print("DevEBox ILI9341");

  // Section title
  tft.setTextSize(1);
  tft.setTextColor(COLOR_YELLOW);
  tft.setCursor(10, 48);
  tft.print("Adafruit_GFX Drawing Functions");

  // Rectangles and lines
  tft.drawRect(10, 65, 140, 105, COLOR_WHITE);
  tft.drawLine(20, 75, 140, 160, COLOR_RED);
  tft.drawLine(140, 75, 20, 160, COLOR_GREEN);
  tft.fillRect(55, 100, 50, 35, COLOR_BLUE);
  tft.drawRect(55, 100, 50, 35, COLOR_WHITE);

  // Circles
  tft.drawCircle(205, 105, 35, COLOR_CYAN);
  tft.fillCircle(205, 105, 20, COLOR_MAGENTA);
  tft.drawCircle(275, 105, 25, COLOR_YELLOW);
  tft.fillCircle(275, 105, 12, COLOR_ORANGE);

  // RGB565 color swatches
  tft.setTextColor(COLOR_WHITE);
  tft.setCursor(10, 183);
  tft.print("RGB565 Colors:");

  const uint16_t colors[] = {
    COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_YELLOW,
    COLOR_CYAN, COLOR_MAGENTA, COLOR_ORANGE, COLOR_WHITE
  };

  for (uint8_t i = 0; i < 8; i++) {
    const int16_t x = 10 + (i * 38);
    tft.fillRect(x, 198, 34, 20, colors[i]);
    tft.drawRect(x, 198, 34, 20, COLOR_WHITE);
  }

  // Text
  tft.setTextSize(1);
  tft.setTextColor(COLOR_GREEN);
  tft.setCursor(10, 226);
  tft.print("Display ready!");
}