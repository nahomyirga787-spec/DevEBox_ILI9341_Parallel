#pragma once

#include <Arduino.h>
#include <SD.h>

class DevEBox_JPEG {

public:

  static const uint8_t MAX_COMPONENTS = 3;
  static const uint8_t MAX_TABLES = 4;
  static const uint8_t MAX_MCU_BLOCKS = 10;

  struct Component {
    uint8_t id;
    uint8_t horizontalSampling;
    uint8_t verticalSampling;
    uint8_t quantizationTable;
    uint8_t dcTable;
    uint8_t acTable;
  };

  struct HuffmanTable {
    bool defined;
    uint8_t codeCounts[16];
    uint8_t symbols[256];
    uint16_t symbolCount;

    int32_t minCode[17];
    int32_t maxCode[17];
    uint16_t valueIndex[17];
  };

  struct ImageInfo {
    uint16_t width;
    uint16_t height;

    uint8_t precision;
    uint8_t componentCount;

    Component components[MAX_COMPONENTS];

    bool quantizationTableDefined[MAX_TABLES];
    uint16_t quantizationTables[MAX_TABLES][64];

    HuffmanTable dcTables[MAX_TABLES];
    HuffmanTable acTables[MAX_TABLES];

    bool restartIntervalDefined;
    uint16_t restartInterval;

    bool baseline;
    bool scanFound;

    // Component indices in the exact order declared by SOS.
    uint8_t scanComponentCount;
    uint8_t scanComponentIndices[MAX_COMPONENTS];
  };

  // One decoded 8x8 spatial block belonging to an MCU.
  struct MCUBlock {
    uint8_t componentIndex;
    uint8_t blockX;
    uint8_t blockY;
    uint8_t pixels[64];
  };

  // Maximum of 10 blocks per JPEG MCU under baseline constraints.
  struct MCU {
    uint8_t blockCount;
    MCUBlock blocks[MAX_MCU_BLOCKS];
  };

  DevEBox_JPEG();

  bool parse(File &file, Stream *diagnostics = nullptr);

  void printInfo(Stream &output) const;

  const ImageInfo &info() const;

  // Entropy decoding.
  void beginEntropy();

  bool readBits(
    File &file,
    uint8_t count,
    uint16_t &value
  );

  bool decodeHuffmanSymbol(
    File &file,
    const HuffmanTable &table,
    uint8_t &symbol
  );

  bool receiveExtend(
    File &file,
    uint8_t category,
    int16_t &value
  );

  // Decode one 8x8 block into natural coefficient order.
  bool decodeBlock(
    File &file,
    uint8_t componentIndex,
    int16_t coefficients[64]
  );

  // Dequantize natural-order coefficients using the component's
  // JPEG quantization table. Results are signed 32-bit values.
  bool dequantizeBlock(
    uint8_t componentIndex,
    const int16_t coefficients[64],
    int32_t dequantized[64]
  ) const;

  // Decode one complete interleaved MCU, including dequantization
  // and IDCT. Blocks are returned in SOS component order, then
  // row-major order within each component's sampling grid.
  //
  // Restart intervals are not supported by this first MCU decoder.
  bool decodeMCU(File &file, MCU &mcu);

  uint16_t mcuColumns() const;
  uint16_t mcuRows() const;
  uint16_t mcuWidth() const;
  uint16_t mcuHeight() const;

private:

  ImageInfo imageInfo_;

  // Entropy bitstream state.
  uint8_t bitBuffer_;
  uint8_t bitsRemaining_;

  uint8_t pendingMarker_;
  bool markerPending_;

  // Independent DC predictors for Y, Cb and Cr.
  int32_t dcPredictors_[MAX_COMPONENTS];

  // File reading.
  bool readByte(File &file, uint8_t &value);
  bool readWord(File &file, uint16_t &value);
  bool readMarker(File &file, uint8_t &marker);
  bool skipBytes(File &file, uint32_t count);

  // JPEG segment parsing.
  bool parseSegment(File &file, uint8_t marker, uint16_t length);
  bool parseDQT(File &file, uint16_t length);
  bool parseDHT(File &file, uint16_t length);
  bool parseSOF0(File &file, uint16_t length);
  bool parseDRI(File &file, uint16_t length);
  bool parseSOS(File &file, uint16_t length);

  // Huffman table construction.
  bool buildHuffmanTable(HuffmanTable &table);

  // Entropy reading.
  bool readEntropyByte(File &file);
  bool readBit(File &file, uint8_t &bit);

  void reset();
};
