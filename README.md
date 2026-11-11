# DevEBox ILI9341 Parallel

An Arduino library for the DevEBox 3.2-inch ILI9341 TFT display using its 16-bit parallel interface.

The initial hardware target is:

- STM32F103C8T6 Blue Pill
- DevEBox ILI9341 320 × 240 TFT, 16-bit parallel bus
- Optional SPI microSD card module
- STM32duino Arduino core
- Adafruit GFX Library

**Project status:** Version `1.0.0`. The original display, SD card, BMP, and JPEG functionality has been demonstrated on the target hardware. The modular library has been organized into reusable components and examples. Continue testing the modular release as changes are made; do not assume every example or configuration has been independently regression-tested.

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
- Chunked SD reads to handle sector boundaries

### JPEG image support

- Custom baseline JPEG decoder
- Baseline sequential JPEG (`SOF0`), 8-bit
- Grayscale and three-component JFIF-style YCbCr images
- 4:4:4, 4:2:2, and 4:2:0 chroma sampling
- MCU-based decoding and direct TFT rendering
- No full-frame image buffer required

### SD card support

- SPI microSD card access
- Image loading from SD card files
- SD directory browsing
- File and directory navigation

## Dependencies

Install the following Arduino library:

- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)

The `SD` and `SPI` libraries are supplied by the selected Arduino core.

The examples use STM32duino-specific SPI pin configuration and AFIO remapping calls. The display driver uses Arduino GPIO APIs, but this project currently targets the STM32 architecture.

## Default Blue Pill wiring

The following pin mapping corresponds to the tested hardware configuration.

### TFT control pins

| TFT signal | Blue Pill pin |
|---|---|
| RS | PA0 |
| RD | PA1 |
| WR | PA2 |
| RST | PA3 |
| BLK | PA12 |

### TFT 16-bit data bus

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

The backlight is configured as active-low by default: `BLK LOW` turns it on, matching the tested module behavior.

### microSD SPI wiring

| SD signal | Blue Pill pin |
|---|---|
| CS | PA8 |
| SCK | PB3 |
| MISO | PB4 |
| MOSI | PB5 |

The SD examples disable JTAG while retaining SWD because PB3, PB4, and PB5 are used for the remapped SPI1 connection.

**Power note:** Confirm the supply-voltage and logic-level requirements of your specific TFT and SD modules before connecting them. Electrical ratings can vary between module revisions.

## Installation

### Install using Arduino IDE

1. Download or clone this repository.
2. Place the library folder inside your Arduino `libraries` directory, or install the repository ZIP through **Sketch → Include Library → Add .ZIP Library**.
3. Install the Adafruit GFX Library through the Arduino Library Manager.
4. Select an STM32F103C8T6 Blue Pill-compatible board configuration.
5. Open an example from **File → Examples → DevEBox ILI9341 Parallel**.

Restart Arduino IDE if the library does not appear immediately.

## Examples

The repository includes example sketches demonstrating the library's main functionality.

| Example | Description |
|---|---|
| `Basic_Display` | Initializes the TFT and demonstrates basic graphics and text. |
| `SD_Card_Test` | Initializes the microSD card and tests file access. |
| `BMP_Display` | Loads and displays a BMP image from the SD card. |
| `BMP_Scaling` | Demonstrates BMP image scaling. |
| `SD_File_Browser` | Demonstrates SD directory navigation and file browsing. |
| `JPEG_Display` | Loads and decodes a baseline JPEG image from the SD card. |
| `Universal_Image_Browser` | Demonstrates browsing and displaying supported image formats. |

Refer to each example sketch for its required SD card files and configuration.

## Basic display example

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

## Image support

### BMP

The BMP decoder supports:

- 24-bit `BI_RGB`
- 32-bit `BI_RGB`
- 32-bit `BI_BITFIELDS`

Both bottom-up and top-down BMP orientations are supported. Images can also be scaled to fit the display, including downscaling larger source images.

### JPEG

The JPEG decoder is a custom implementation designed for memory-constrained microcontrollers. It decodes baseline sequential JPEG images and renders image data to the TFT as it is decoded.

Supported JPEG features:

- 8-bit baseline sequential JPEG (`SOF0`)
- Grayscale images
- Three-component JFIF-style YCbCr images
- 4:4:4, 4:2:2, and 4:2:0 sampling

The decoder does not require a full-frame framebuffer. It processes image data in minimum coded units (MCUs), reducing memory requirements.

**JPEG limitations:**

- Progressive JPEG is not supported.
- Lossless JPEG is not supported.
- Arithmetic-coded JPEG is not supported.
- Nonzero restart intervals are not currently supported.
- Three-component color decoding expects JFIF component identifiers 1, 2, and 3.

For the most predictable results, use baseline sequential JPEG files.

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
│   └── ...
│
├── examples/
│   ├── Basic_Display/
│   ├── SD_Card_Test/
│   ├── BMP_Display/
│   ├── BMP_Scaling/
│   ├── SD_File_Browser/
│   ├── JPEG_Display/
│   └── Universal_Image_Browser/
│
├── library.properties
├── LICENSE
└── README.md
```

The source directory may contain additional implementation and helper files as the library develops.

## Performance and memory considerations

The display driver uses GPIO operations to communicate with the 16-bit parallel bus. This approach prioritizes portability and straightforward hardware control rather than maximum possible throughput.

The JPEG decoder processes image data incrementally instead of allocating a full image framebuffer. This makes it more suitable for memory-constrained microcontrollers such as the STM32F103C8T6.

Actual performance depends on image dimensions, JPEG sampling configuration, SD card speed, and the display's GPIO write speed.

## Known limitations

- The initial hardware target is the STM32F103C8T6 Blue Pill and the described DevEBox 320 × 240 ILI9341 16-bit parallel module.
- The display driver uses GPIO-based parallel communication.
- BMP support is limited to 24-bit `BI_RGB` and 32-bit `BI_RGB` / `BI_BITFIELDS`.
- JPEG support is limited to the baseline sequential formats described above.
- Progressive, lossless, and arithmetic-coded JPEG are not supported.
- JPEG restart intervals are not currently supported.
- Touch input is not implemented.
- Hardware compatibility with other boards and display modules has not been established.

## Contributing

Contributions, bug reports, and suggestions are welcome. If reporting a hardware issue, include:

- Microcontroller and board configuration
- Display module and interface
- SD card module and wiring, if applicable
- Arduino IDE and STM32duino core versions
- Example sketch and relevant Serial Monitor output

## License

This project is released under the MIT License. See [LICENSE](LICENSE) for details.
