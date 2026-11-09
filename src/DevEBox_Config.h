#pragma once

#include <Arduino.h>

/*
 * Default pin map for:
 *   STM32F103C8T6 Blue Pill
 *   DevEBox ILI9341 3.2-inch TFT, 16-bit parallel interface
 *
 * Override these macros before including DevEBox_ILI9341.h,
 * or edit this file for your own wiring.
 */

#ifndef DEVBOX_TFT_WIDTH
#define DEVBOX_TFT_WIDTH 320
#endif

#ifndef DEVBOX_TFT_HEIGHT
#define DEVBOX_TFT_HEIGHT 240
#endif

#ifndef DEVBOX_LCD_RS
#define DEVBOX_LCD_RS PA0
#endif

#ifndef DEVBOX_LCD_RD
#define DEVBOX_LCD_RD PA1
#endif

#ifndef DEVBOX_LCD_WR
#define DEVBOX_LCD_WR PA2
#endif

#ifndef DEVBOX_LCD_RST
#define DEVBOX_LCD_RST PA3
#endif

#ifndef DEVBOX_LCD_BLK
#define DEVBOX_LCD_BLK PA12
#endif

#ifndef DEVBOX_BACKLIGHT_ACTIVE_LOW
#define DEVBOX_BACKLIGHT_ACTIVE_LOW 1
#endif

#ifndef DEVBOX_DB00
#define DEVBOX_DB00 PB0
#endif
#ifndef DEVBOX_DB01
#define DEVBOX_DB01 PB1
#endif
#ifndef DEVBOX_DB02
#define DEVBOX_DB02 PA7
#endif
#ifndef DEVBOX_DB03
#define DEVBOX_DB03 PB6
#endif
#ifndef DEVBOX_DB04
#define DEVBOX_DB04 PB7
#endif
#ifndef DEVBOX_DB05
#define DEVBOX_DB05 PB8
#endif
#ifndef DEVBOX_DB06
#define DEVBOX_DB06 PB9
#endif
#ifndef DEVBOX_DB07
#define DEVBOX_DB07 PB10
#endif
#ifndef DEVBOX_DB08
#define DEVBOX_DB08 PB11
#endif
#ifndef DEVBOX_DB09
#define DEVBOX_DB09 PB12
#endif
#ifndef DEVBOX_DB10
#define DEVBOX_DB10 PB13
#endif
#ifndef DEVBOX_DB11
#define DEVBOX_DB11 PB14
#endif
#ifndef DEVBOX_DB12
#define DEVBOX_DB12 PB15
#endif
#ifndef DEVBOX_DB13
#define DEVBOX_DB13 PA4
#endif
#ifndef DEVBOX_DB14
#define DEVBOX_DB14 PA5
#endif
#ifndef DEVBOX_DB15
#define DEVBOX_DB15 PA6
#endif

static const uint32_t DEVBOX_DEFAULT_DATA_PINS[16] = {
  DEVBOX_DB00, DEVBOX_DB01, DEVBOX_DB02, DEVBOX_DB03,
  DEVBOX_DB04, DEVBOX_DB05, DEVBOX_DB06, DEVBOX_DB07,
  DEVBOX_DB08, DEVBOX_DB09, DEVBOX_DB10, DEVBOX_DB11,
  DEVBOX_DB12, DEVBOX_DB13, DEVBOX_DB14, DEVBOX_DB15
};
