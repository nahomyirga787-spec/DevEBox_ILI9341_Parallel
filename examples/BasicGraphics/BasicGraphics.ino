#include <DevEBox_ILI9341.h>

DevEBox_ILI9341 tft;

void setup() {
  tft.begin();
  tft.fillScreen(0x0000);

  tft.setTextColor(0xFFFF);
  tft.setTextSize(2);
  tft.setCursor(12, 12);
  tft.println("DevEBox ILI9341");

  tft.drawRect(10, 50, 140, 80, 0xF800);
  tft.fillCircle(220, 90, 35, 0x07E0);
  tft.drawLine(10, 160, 300, 220, 0x001F);
}

void loop() {
}
