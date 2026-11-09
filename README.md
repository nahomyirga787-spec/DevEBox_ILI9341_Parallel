# DevEBox ILI9341 Parallel

An Arduino library for the DevEBox 3.2-inch ILI9341 TFT using its 16-bit parallel interface.

The initial hardware target is:

- STM32F103C8T6 Blue Pill
- DevEBox ILI9341 320 x 240 TFT, 16-bit parallel bus
- Optional SPI microSD module
- STM32duino Arduino core
- Adafruit GFX Library

> **Status:** Initial modular release (`1.0.0`). The original monolithic sketch was demonstrated on the target hardware. The modular library has been reorganized from that sketch and still needs to be compiled and regression-tested on the target before being treated as a verified release.

## Features

- 16-bit parallel ILI9341 communication
- Adafruit_GFX-compatible drawing API
- Text, lines, rectangles, circles and other inherited GFX primitives
- Efficient RGB565 horizontal pixel streaming
- Optional BMP reader for SD `File` objects
- 24-bit uncompressed BMP (`BI_RGB`)
- 32-bit uncompressed BMP (`BI_RGB`)
- 32-bit BMP with RGB bitfields (`BI_BITFIELDS`)
- Bottom-up and top-down BMP orientation
- Sector-boundary-aware SD reads

## Dependencies

Install these Arduino libraries:

- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
- The `SD` and `SPI` libraries supplied by the selected Arduino core

The examples use STM32duino-specific SPI pin configuration and AFIO remapping calls. The display driver itself is written against Arduino GPIO APIs, but this library release is currently declared for the STM32 architecture because that is the tested target family.

## Default Blue Pill wiring

### TFT control pins

| TFT signal | Blue Pill |
|---|---|
| RS | PA0 |
| RD | PA1 |
| WR | PA2 |
| RST | PA3 |
| BLK | PA12 |

### TFT 16-bit data bus

| TFT signal | Blue Pill | TFT signal | Blue Pill |
|---|---|---|---|
| DB00 | PB0 | DB08 | PB11 |
| DB01 | PB1 | DB09 | PB12 |
| DB02 | PA7 | DB10 | PB13 |
| DB03 | PB6 | DB11 | PB14 |
| DB04 | PB7 | DB12 | PB15 |
| DB05 | PB8 | DB13 | PA4 |
| DB06 | PB9 | DB14 | PA5 |
| DB07 | PB10 | DB15 | PA6 |

The backlight is configured active-low by default (`BLK LOW` turns it on), matching the tested module behavior.

### microSD SPI wiring

| SD signal | Blue Pill |
|---|---|
| CS | PA8 |
| SCK | PB3 |
| MISO | PB4 |
| MOSI | PB5 |

The SD example disables JTAG while retaining SWD because PB3/PB4/PB5 are used for remapped SPI1.

**Power note:** Confirm the supply and logic-level requirements of your specific TFT and SD modules before wiring. This library does not establish their electrical ratings.

## Basic graphics example

```cpp
#include <DevEBox_ILI9341.h>

DevEBox_ILI9341 tft;

void setup() {
  tft.begin();
  tft.fillScreen(0x0000);

  tft.setTextColor(0xFFFF);
  tft.setTextSize(2);
  tft.setCursor(12, 12);
  tft.println("Hello, DevEBox!");

  tft.drawRect(10, 50, 140, 80, 0xF800);
  tft.fillCircle(220, 90, 35, 0x07E0);
}

void loop() {}
```

## BMP image viewer

Place a supported BMP file named `IMAGE.BMP` in the root directory of a FAT-formatted SD card. The image dimensions must not exceed 320 x 240 with the default display configuration.

```cpp
#include <SPI.h>
#include <SD.h>
#include <DevEBox_ILI9341.h>
#include <DevEBox_BMP.h>

#define SD_CS PA8

DevEBox_ILI9341 tft;

void setup() {
  Serial1.begin(115200);

  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_AFIO_REMAP_SWJ_NOJTAG();

  SPI.setMOSI(PB5);
  SPI.setMISO(PB4);
  SPI.setSCLK(PB3);

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  SPI.begin();

  tft.begin();

  if (!SD.begin(SD_CS)) {
    Serial1.println("SD initialization failed.");
    return;
  }

  File image = SD.open("/IMAGE.BMP", FILE_READ);
  if (!image) {
    Serial1.println("IMAGE.BMP not found.");
    return;
  }

  bool ok = DevEBox_BMP::draw(tft, image, &Serial1);
  image.close();

  Serial1.println(ok ? "BMP displayed." : "BMP display failed.");
}

void loop() {}
```

## Configuration

The default pin map is in `src/DevEBox_Config.h`. You can edit the pin macros there to match your wiring.

The library currently assumes a 16-bit data bus and the ILI9341 initialization sequence used by the tested module. Changing the pin map does not make an electrically different display interface compatible.

## Repository layout

```text
DevEBox_ILI9341_Parallel/
├── src/
│   ├── DevEBox_Config.h
│   ├── DevEBox_ILI9341.h
│   ├── DevEBox_ILI9341.cpp
│   ├── DevEBox_BMP.h
│   └── DevEBox_BMP.cpp
├── examples/
│   ├── BasicGraphics/
│   ├── SD_Card_Test/
│   └── BMP_Image_Viewer/
├── library.properties
├── LICENSE
└── README.md
```

## Installing from ZIP

1. Download the repository ZIP.
2. In Arduino IDE, select **Sketch → Include Library → Add .ZIP Library**.
3. Select the ZIP.
4. Open one of the examples from **File → Examples → DevEBox ILI9341 Parallel**.

## Known limitations

- Initial target is STM32F103C8T6 Blue Pill and the described DevEBox 16-bit parallel ILI9341 module.
- The parallel driver uses `digitalWrite()` for each data bit; it prioritizes compatibility and correctness over maximum throughput.
- BMP support is limited to 24-bit BI_RGB and 32-bit BI_RGB/BI_BITFIELDS.
- BMP files larger than the display are rejected rather than scaled.
- JPEG, PNG, touch input, and directory browsing are not included.
- This modular version must be compiled and tested on the physical target before publishing a stable release.

## License

MIT. See `LICENSE`.
