# DevEBox ILI9341 Parallel

An Arduino library for the **DevEBox 3.2-inch ILI9341 TFT display** using its 16-bit parallel interface.

## Target hardware

- STM32F103C8T6 Blue Pill
- DevEBox ILI9341 320 × 240 TFT, 16-bit parallel bus
- Optional SPI microSD card module
- STM32duino Arduino core
- Adafruit GFX Library
- SdFat library

**Project version:** 1.0.0

The library provides a modular display driver, BMP and baseline JPEG image decoding, image scaling, and microSD file browsing. The library components and six examples have been tested on the target Blue Pill hardware. Compatibility with other boards and display modules has not yet been established.

## Features

### Display driver

- 16-bit parallel communication with the ILI9341 controller
- Adafruit_GFX-compatible drawing API
- Text, lines, rectangles, circles, and other inherited GFX primitives
- RGB565 color support
- Horizontal pixel streaming

### BMP image support

- 24-bit uncompressed BMP (`BI_RGB`)
- 32-bit uncompressed BMP (`BI_RGB`)
- 32-bit BMP with RGB bitfields (`BI_BITFIELDS`)
- Bottom-up and top-down BMP orientation
- RGB mask extraction and RGB888-to-RGB565 conversion
- Image scaling, including downscaling images larger than the display
- Centered rendering and display-boundary clipping
- Chunked SD reads

### JPEG image support

- Custom baseline JPEG decoder
- Baseline sequential JPEG (`SOF0`), 8-bit
- Grayscale and three-component JFIF-style YCbCr images
- 4:4:4, 4:2:2, and 4:2:0 chroma sampling
- MCU-based decoding and incremental TFT rendering
- Fixed-point IDCT and color conversion
- JPEG restart interval support
- No full-frame image buffer required

Restart interval decoding has been demonstrated successfully with a JPEG containing a DRI interval of 100 MCUs.

### microSD support

- SPI microSD card access through SdFat
- File and directory browsing
- Directory navigation
- Opening BMP and JPEG images from the card
- Long filename support through the configured SdFat filesystem

### Image scaling

Both BMP and JPEG renderers support image scaling.

The Universal Serial Image Browser provides a shared scale setting for both formats. Changing the scale while an image is displayed automatically redraws that image.

The universal browser accepts scale values from 0.1× to 4.0×.

## Dependencies

Install the following Arduino libraries:

- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
- [SdFat](https://github.com/greiman/SdFat)

The SPI library is supplied by the selected Arduino core.

The examples use STM32duino-specific SPI pin configuration and AFIO remapping calls. This project currently targets the STM32 architecture.

## Hardware configuration

### Default Blue Pill wiring

The following pin mapping corresponds to the tested hardware configuration.

#### TFT control pins

| TFT signal | Blue Pill pin |
|---|---|
| RS | PA0 |
| RD | PA1 |
| WR | PA2 |
| RST | PA3 |
| BLK | PA12 |

#### TFT 16-bit data bus

| TFT signal | Blue Pill pin | TFT signal | Blue Pill pin |
|---|---|---|---|
| DB00 | PB0 | DB08 | PB11 |
| DB01 | PB1 | DB09 | PB12 |
| DB02 | PA7 | DB10 | PB13 |
| DB03 | PB6 | DB11 | PB14 |
| DB04 | PB7 | DB12 | PB15 |
| DB05 | PB8 | DB13 | PA4 |
| DB06 | PB9 | DB14 | PA5 |
| DB07 | PB10 | DB15 | PA6 |

The backlight is configured as active-low by default. On the tested module, BLK LOW turns the backlight on.

#### microSD SPI wiring

| SD signal | Blue Pill pin |
|---|---|
| CS | PA8 |
| SCK | PB3 |
| MISO | PB4 |
| MOSI | PB5 |

The SD examples use the remapped SPI1 pins. JTAG is disabled while retaining SWD debugging because PB3 and PB4 are otherwise associated with JTAG functions.

**Power and logic-level note:** Confirm the supply-voltage and logic-level requirements of your particular TFT and SD modules before connecting them. Electrical ratings can vary between module revisions.

## Installation

### Arduino IDE

1. Download or clone this repository.
2. Place the library folder inside your Arduino `libraries` directory, or install the repository ZIP using **Sketch → Include Library → Add .ZIP Library**.
3. Install the Adafruit GFX Library through Library Manager.
4. Install SdFat through Library Manager.
5. Select an STM32F103C8T6 Blue Pill-compatible board configuration.
6. Open an example through **File → Examples → DevEBox ILI9341 Parallel**.

Restart Arduino IDE if the library does not appear immediately.

For the image examples, configure the microSD card with the required image files in its root directory, or adjust the filename in the example.

## Examples

The repository includes six examples.

| Example | Description |
|---|---|
| `Basic_Display` | Initializes the TFT and demonstrates graphics, text, and RGB565 colors. |
| `BMP_Image_Viewer` | Loads and displays a BMP image from the microSD card. |
| `BMP_Scaling` | Demonstrates BMP image scaling. |
| `JPEG_Image_Viewer` | Loads and displays a baseline JPEG image from the microSD card. |
| `JPEG_Scaling` | Demonstrates JPEG image scaling. |
| `Universal_Serial_Image_Browser` | Provides serial file browsing, directory navigation, BMP/JPEG viewing, and shared image scaling with automatic redraw. |

### Universal Serial Image Browser commands

The universal browser uses `Serial1` at 115200 baud.

| Command | Description |
|---|---|
| `ls` | List directory contents |
| `cd FOLDER` | Enter a directory |
| `cd /` | Return to the root directory |
| `up` | Move to the parent directory |
| `pwd` | Display the current directory |
| `open IMAGE.BMP` | Open a BMP image |
| `open IMAGE.JPG` | Open a JPEG image |
| `scale` | Display the current scale |
| `scale 0.5` | Set scale to 0.5× |
| `scale 1.0` | Set scale to 1.0× |
| `scale 2.0` | Set scale to 2.0× |
| `help` | Display available commands |

Scaling applies to both BMP and JPEG images. Changing the scale automatically redraws the currently displayed image.

## Basic display example

```cpp
#include <Arduino.h>
#include "DevEBox_ILI9341.h"

#define LCD_BACKLIGHT PA12

DevEBox_ILI9341 tft;

void setup() {
  pinMode(LCD_BACKLIGHT, OUTPUT);
  digitalWrite(LCD_BACKLIGHT, LOW);

  tft.begin();
  tft.setBacklight(true);

  tft.fillScreen(0x0000);

  tft.setTextColor(0xFFFF);
  tft.setTextSize(2);
  tft.setCursor(12, 12);
  tft.println("Hello, DevEBox!");

  tft.drawRect(10, 50, 140, 80, 0xF800);
  tft.fillCircle(220, 90, 35, 0x07E0);
}

void loop() {
}
```

## Image format details

### BMP

The BMP decoder supports:

- 24-bit `BI_RGB`
- 32-bit `BI_RGB`
- 32-bit `BI_BITFIELDS`

Both bottom-up and top-down BMP orientations are supported. Images can be scaled, centered, and clipped to the display boundaries.

### JPEG

The JPEG decoder is a custom implementation designed for memory-constrained microcontrollers. It decodes baseline sequential JPEG images and renders image data incrementally.

Supported features:

- 8-bit baseline sequential JPEG (`SOF0`)
- Grayscale images
- Three-component JFIF-style YCbCr images
- 4:4:4, 4:2:2, and 4:2:0 sampling
- Restart interval decoding

The decoder processes image data in minimum coded units (MCUs), avoiding the need for a full-frame framebuffer.

**JPEG limitations:**

- Progressive JPEG is not supported.
- Lossless JPEG is not supported.
- Arithmetic-coded JPEG is not supported.
- Three-component color decoding expects JFIF component identifiers 1, 2, and 3.

For predictable results, use baseline sequential JPEG files.

## Configuration

The default display pin mapping is defined in:

`src/DevEBox_Config.h`

You can modify the pin configuration to match your wiring.

The driver assumes a 16-bit parallel data bus and the ILI9341 initialization sequence used by the target module. Changing the pin mapping does not make an electrically different display interface compatible.

## Repository structure

```text
DevEBox_ILI9341_Parallel/
│
├── src/
│   ├── DevEBox_Config.h
│   ├── DevEBox_ILI9341.h
│   ├── DevEBox_ILI9341.cpp
│   ├── DevEBox_BMP.h
│   ├── DevEBox_BMP.cpp
│   ├── DevEBox_JPEG.h
│   ├── DevEBox_JPEG.cpp
│   ├── DevEBox_JPEG_Color.h
│   ├── DevEBox_JPEG_Color.cpp
│   ├── DevEBox_JPEG_Display.h
│   ├── DevEBox_JPEG_Display.cpp
│   ├── DevEBox_IDCT.h
│   ├── DevEBox_IDCT.cpp
│   ├── DevEBox_SD_Browser.h
│   └── DevEBox_SD_Browser.cpp
│
├── examples/
│   ├── Basic_Display/
│   ├── BMP_Image_Viewer/
│   ├── BMP_Scaling/
│   ├── JPEG_Image_Viewer/
│   ├── JPEG_Scaling/
│   └── Universal_Serial_Image_Browser/
│
├── library.properties
├── LICENSE
└── README.md
```

## Performance and memory considerations

The display driver uses GPIO operations to communicate with the 16-bit parallel bus. This approach prioritizes straightforward hardware control rather than maximum possible throughput.

The JPEG decoder processes image data incrementally instead of allocating a full image framebuffer. This reduces memory requirements on microcontrollers such as the STM32F103C8T6.

Actual performance depends on image dimensions, JPEG sampling configuration, SD card speed, and display GPIO write speed.

The STM32F103C8T6 has limited Flash and SRAM resources. When compiling the larger image-decoding examples, the Arduino IDE's optimization settings can affect whether the firmware fits in available Flash.

## Known limitations

- The initial hardware target is the STM32F103C8T6 Blue Pill and the described DevEBox 320 × 240 ILI9341 16-bit parallel module.
- The display driver uses GPIO-based parallel communication.
- BMP support is limited to the documented 24-bit and 32-bit formats.
- JPEG support is limited to the documented baseline sequential formats.
- Progressive, lossless, and arithmetic-coded JPEG are not supported.
- Touch input is not implemented.
- Hardware compatibility with other boards and display modules has not been established.

## Contributing

Contributions, bug reports, and suggestions are welcome.

When reporting a hardware issue, include:

- Microcontroller and board configuration
- Display module and interface
- SD card module and wiring, if applicable
- Arduino IDE and STM32duino core versions
- Example sketch and relevant Serial Monitor output

## License

This project is released under the MIT License. See [LICENSE](LICENSE) for details.