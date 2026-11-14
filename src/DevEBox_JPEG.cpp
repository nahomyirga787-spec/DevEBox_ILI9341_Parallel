
#include "DevEBox_JPEG.h"
#include "DevEBox_IDCT.h"
#include <string.h>

// JPEG zigzag position to natural 8x8 coefficient position.
static const uint8_t ZIGZAG[64] = {
   0,  1,  8, 16,  9,  2,  3, 10,
  17, 24, 32, 25, 18, 11,  4,  5,
  12, 19, 26, 33, 40, 48, 41, 34,
  27, 20, 13,  6,  7, 14, 21, 28,
  35, 42, 49, 56, 57, 50, 43, 36,
  29, 22, 15, 23, 30, 37, 44, 51,
  58, 59, 52, 45, 38, 31, 39, 46,
  53, 60, 61, 54, 47, 55, 62, 63
};


// ============================================================
// CONSTRUCTOR AND RESET
// ============================================================

DevEBox_JPEG::DevEBox_JPEG() {
  reset();
}

void DevEBox_JPEG::reset() {

  memset(&imageInfo_, 0, sizeof(imageInfo_));

  memset(dcPredictors_, 0, sizeof(dcPredictors_));

  decodedMCUs_ = 0;
  expectedRestartMarker_ = 0;

  beginEntropy();
}


// ============================================================
// BASIC FILE READING
// ============================================================

bool DevEBox_JPEG::readByte(File &file, uint8_t &value) {

  int result = file.read();

  if (result < 0) {
    return false;
  }

  value = (uint8_t)result;
  return true;
}


bool DevEBox_JPEG::readWord(File &file, uint16_t &value) {

  uint8_t highByte;
  uint8_t lowByte;

  if (!readByte(file, highByte) ||
      !readByte(file, lowByte)) {
    return false;
  }

  value = ((uint16_t)highByte << 8) | lowByte;

  return true;
}


bool DevEBox_JPEG::skipBytes(File &file, uint32_t count) {

  uint32_t position = file.position();
  uint32_t size = file.size();

  if (position > size || count > size - position) {
    return false;
  }

  return file.seek(position + count);
}


// ============================================================
// JPEG MARKER READING
// ============================================================

bool DevEBox_JPEG::readMarker(File &file, uint8_t &marker) {

  uint8_t value;

  while (true) {

    if (!readByte(file, value)) {
      return false;
    }

    if (value == 0xFF) {
      break;
    }
  }

  do {

    if (!readByte(file, value)) {
      return false;
    }

  } while (value == 0xFF);

  if (value == 0x00) {
    return false;
  }

  marker = value;
  return true;
}


// ============================================================
// CANONICAL HUFFMAN TABLE CONSTRUCTION
// ============================================================

bool DevEBox_JPEG::buildHuffmanTable(HuffmanTable &table) {

  uint32_t code = 0;
  uint16_t symbolIndex = 0;

  for (uint8_t length = 1; length <= 16; length++) {

    uint8_t count = table.codeCounts[length - 1];

    table.valueIndex[length] = symbolIndex;

    if (count == 0) {

      table.minCode[length] = -1;
      table.maxCode[length] = -1;

    }
    else {

      uint32_t availableCodes = 1UL << length;

      if (code + count > availableCodes) {
        return false;
      }

      table.minCode[length] = (int32_t)code;
      table.maxCode[length] = (int32_t)(code + count - 1);

      // JPEG reserves the all-ones code.
      if ((uint32_t)table.maxCode[length] ==
          availableCodes - 1) {
        return false;
      }

      symbolIndex += count;
    }

    code = (code + count) << 1;
  }

  return (
    symbolIndex == table.symbolCount &&
    symbolIndex > 0 &&
    symbolIndex <= 256
  );
}


// ============================================================
// DQT - QUANTIZATION TABLES
// ============================================================

bool DevEBox_JPEG::parseDQT(File &file, uint16_t length) {

  if (length < 3) {
    return false;
  }

  uint16_t remaining = length - 2;

  while (remaining > 0) {

    uint8_t tableInfo;

    if (!readByte(file, tableInfo)) {
      return false;
    }

    remaining--;

    uint8_t precision = tableInfo >> 4;
    uint8_t tableID = tableInfo & 0x0F;

    if (tableID >= MAX_TABLES || precision != 0) {
      return false;
    }

    if (remaining < 64) {
      return false;
    }

    for (uint8_t i = 0; i < 64; i++) {

      uint8_t value;

      if (!readByte(file, value)) {
        return false;
      }

      imageInfo_.quantizationTables[tableID][i] = value;
    }

    remaining -= 64;
    imageInfo_.quantizationTableDefined[tableID] = true;
  }

  return true;
}


// ============================================================
// DHT - HUFFMAN TABLES
// ============================================================

bool DevEBox_JPEG::parseDHT(File &file, uint16_t length) {

  if (length < 2) {
    return false;
  }

  uint16_t remaining = length - 2;

  while (remaining > 0) {

    uint8_t tableInfo;

    if (!readByte(file, tableInfo)) {
      return false;
    }

    remaining--;

    uint8_t tableClass = tableInfo >> 4;
    uint8_t tableID = tableInfo & 0x0F;

    if (tableClass > 1 || tableID >= MAX_TABLES) {
      return false;
    }

    HuffmanTable &table =
      (tableClass == 0)
      ? imageInfo_.dcTables[tableID]
      : imageInfo_.acTables[tableID];

    memset(&table, 0, sizeof(HuffmanTable));

    if (remaining < 16) {
      return false;
    }

    uint16_t symbolCount = 0;

    for (uint8_t i = 0; i < 16; i++) {

      if (!readByte(file, table.codeCounts[i])) {
        return false;
      }

      symbolCount += table.codeCounts[i];
    }

    remaining -= 16;

    if (symbolCount == 0 ||
        symbolCount > 256 ||
        remaining < symbolCount) {
      return false;
    }

    table.symbolCount = symbolCount;

    for (uint16_t i = 0; i < symbolCount; i++) {

      if (!readByte(file, table.symbols[i])) {
        return false;
      }
    }

    remaining -= symbolCount;

    if (!buildHuffmanTable(table)) {
      return false;
    }

    table.defined = true;
  }

  return true;
}


// ============================================================
// SOF0 - BASELINE FRAME
// ============================================================

bool DevEBox_JPEG::parseSOF0(File &file, uint16_t length) {

  if (length < 11) {
    return false;
  }

  uint8_t precision;
  uint16_t height;
  uint16_t width;
  uint8_t componentCount;

  if (!readByte(file, precision) ||
      !readWord(file, height) ||
      !readWord(file, width) ||
      !readByte(file, componentCount)) {
    return false;
  }

  if ((componentCount != 1 && componentCount != 3) ||
      length != 8 + 3 * componentCount ||
      precision != 8 ||
      width == 0 ||
      height == 0) {
    return false;
  }

  imageInfo_.width = width;
  imageInfo_.height = height;
  imageInfo_.precision = precision;
  imageInfo_.componentCount = componentCount;

  uint16_t totalBlocks = 0;

  for (uint8_t i = 0; i < componentCount; i++) {

    Component &component = imageInfo_.components[i];

    uint8_t sampling;

    if (!readByte(file, component.id) ||
        !readByte(file, sampling)) {
      return false;
    }

    component.horizontalSampling = sampling >> 4;
    component.verticalSampling = sampling & 0x0F;

    if (component.horizontalSampling == 0 ||
        component.verticalSampling == 0 ||
        component.horizontalSampling > 4 ||
        component.verticalSampling > 4) {
      return false;
    }

    totalBlocks +=
      component.horizontalSampling *
      component.verticalSampling;

    if (!readByte(file, component.quantizationTable)) {
      return false;
    }

    if (component.quantizationTable >= MAX_TABLES) {
      return false;
    }

    for (uint8_t j = 0; j < i; j++) {

      if (imageInfo_.components[j].id == component.id) {
        return false;
      }
    }
  }

  if (totalBlocks > 10) {
    return false;
  }

  imageInfo_.baseline = true;
  return true;
}


// ============================================================
// DRI - RESTART INTERVAL
// ============================================================

bool DevEBox_JPEG::parseDRI(File &file, uint16_t length) {

  if (length != 4) {
    return false;
  }

  if (!readWord(file, imageInfo_.restartInterval)) {
    return false;
  }

  imageInfo_.restartIntervalDefined = true;
  return true;
}


// ============================================================
// SOS - START OF SCAN
// ============================================================

bool DevEBox_JPEG::parseSOS(File &file, uint16_t length) {

  if (!imageInfo_.baseline) {
    return false;
  }

  uint8_t scanComponentCount;

  if (!readByte(file, scanComponentCount)) {
    return false;
  }

  if (scanComponentCount != imageInfo_.componentCount ||
      length != 6 + 2 * scanComponentCount) {
    return false;
  }

  bool componentSeen[MAX_COMPONENTS] = { false };

  imageInfo_.scanComponentCount = scanComponentCount;

  for (uint8_t i = 0; i < scanComponentCount; i++) {

    uint8_t componentID;
    uint8_t tableSelector;

    if (!readByte(file, componentID) ||
        !readByte(file, tableSelector)) {
      return false;
    }

    uint8_t dcTable = tableSelector >> 4;
    uint8_t acTable = tableSelector & 0x0F;

    if (dcTable >= MAX_TABLES || acTable >= MAX_TABLES) {
      return false;
    }

    bool found = false;

    for (uint8_t j = 0; j < imageInfo_.componentCount; j++) {

      Component &component = imageInfo_.components[j];

      if (component.id == componentID) {

        if (componentSeen[j]) {
          return false;
        }

        componentSeen[j] = true;
        imageInfo_.scanComponentIndices[i] = j;

        component.dcTable = dcTable;
        component.acTable = acTable;

        found = true;
        break;
      }
    }

    if (!found) {
      return false;
    }

    if (!imageInfo_.dcTables[dcTable].defined ||
        !imageInfo_.acTables[acTable].defined) {
      return false;
    }
  }

  for (uint8_t i = 0; i < imageInfo_.componentCount; i++) {

    if (!componentSeen[i]) {
      return false;
    }

    uint8_t qTable =
      imageInfo_.components[i].quantizationTable;

    if (!imageInfo_.quantizationTableDefined[qTable]) {
      return false;
    }
  }

  uint8_t spectralStart;
  uint8_t spectralEnd;
  uint8_t successiveApproximation;

  if (!readByte(file, spectralStart) ||
      !readByte(file, spectralEnd) ||
      !readByte(file, successiveApproximation)) {
    return false;
  }

  if (spectralStart != 0 ||
      spectralEnd != 63 ||
      successiveApproximation != 0) {
    return false;
  }

  imageInfo_.scanFound = true;
  return true;
}


// ============================================================
// SEGMENT DISPATCHER
// ============================================================

bool DevEBox_JPEG::parseSegment(
  File &file,
  uint8_t marker,
  uint16_t length
) {

  if (length < 2) {
    return false;
  }

  switch (marker) {

    case 0xDB:
      return parseDQT(file, length);

    case 0xC4:
      return parseDHT(file, length);

    case 0xC0:
      return parseSOF0(file, length);

    case 0xDD:
      return parseDRI(file, length);

    case 0xDA:
      return parseSOS(file, length);

    default:
      return skipBytes(file, length - 2);
  }
}


// ============================================================
// MAIN JPEG HEADER PARSER
// ============================================================

bool DevEBox_JPEG::parse(File &file, Stream *diagnostics) {

  (void)diagnostics;

  reset();

  if (!file || !file.seek(0)) {
    return false;
  }

  uint8_t marker;

  if (!readMarker(file, marker) || marker != 0xD8) {
    return false;
  }

  while (file.available()) {

    if (!readMarker(file, marker)) {
      return false;
    }

    if (marker == 0xD9) {
      return false;
    }

    if (marker == 0xD8 ||
        marker == 0x01 ||
        (marker >= 0xD0 && marker <= 0xD7)) {
      continue;
    }

    if (marker >= 0xC0 &&
        marker <= 0xCF &&
        marker != 0xC0 &&
        marker != 0xC4 &&
        marker != 0xC8 &&
        marker != 0xCC) {
      return false;
    }

    uint16_t segmentLength;

    if (!readWord(file, segmentLength) ||
        segmentLength < 2) {
      return false;
    }

    if (!parseSegment(file, marker, segmentLength)) {
      return false;
    }

    if (marker == 0xDA) {
      return imageInfo_.scanFound;
    }
  }

  return false;
}


// ============================================================
// ENTROPY INITIALIZATION
// ============================================================

void DevEBox_JPEG::beginEntropy() {

  bitBuffer_ = 0;
  bitsRemaining_ = 0;

  pendingMarker_ = 0;
  markerPending_ = false;

  memset(dcPredictors_, 0, sizeof(dcPredictors_));

  decodedMCUs_ = 0;
  expectedRestartMarker_ = 0;
}


// ============================================================
// READ ENTROPY BYTE
// ============================================================

bool DevEBox_JPEG::readEntropyByte(File &file) {

  if (markerPending_) {
    return false;
  }

  int result = file.read();

  if (result < 0) {
    return false;
  }

  uint8_t value = (uint8_t)result;

  if (value != 0xFF) {
    bitBuffer_ = value;
    return true;
  }

  do {

    result = file.read();

    if (result < 0) {
      return false;
    }

    value = (uint8_t)result;

  } while (value == 0xFF);

  if (value == 0x00) {
    bitBuffer_ = 0xFF;
    return true;
  }

  pendingMarker_ = value;
  markerPending_ = true;

  return false;
}


// ============================================================
// READ ONE BIT
// ============================================================

bool DevEBox_JPEG::readBit(File &file, uint8_t &bit) {

  if (bitsRemaining_ == 0) {

    if (!readEntropyByte(file)) {
      return false;
    }

    bitsRemaining_ = 8;
  }

  bit = (bitBuffer_ >> 7) & 1;

  bitBuffer_ <<= 1;
  bitsRemaining_--;

  return true;
}


// ============================================================
// READ MULTIPLE BITS
// ============================================================

bool DevEBox_JPEG::readBits(
  File &file,
  uint8_t count,
  uint16_t &value
) {

  if (count > 16) {
    return false;
  }

  value = 0;

  for (uint8_t i = 0; i < count; i++) {

    uint8_t bit;

    if (!readBit(file, bit)) {
      return false;
    }

    value = (value << 1) | bit;
  }

  return true;
}


// ============================================================
// HUFFMAN SYMBOL DECODER
// ============================================================

bool DevEBox_JPEG::decodeHuffmanSymbol(
  File &file,
  const HuffmanTable &table,
  uint8_t &symbol
) {

  if (!table.defined || table.symbolCount == 0) {
    return false;
  }

  uint32_t code = 0;

  for (uint8_t length = 1; length <= 16; length++) {

    uint8_t bit;

    if (!readBit(file, bit)) {
      return false;
    }

    code = (code << 1) | bit;

    int32_t minimum = table.minCode[length];
    int32_t maximum = table.maxCode[length];

    if (minimum < 0 || maximum < 0) {
      continue;
    }

    if (code >= (uint32_t)minimum &&
        code <= (uint32_t)maximum) {

      uint32_t index =
        table.valueIndex[length] +
        (code - (uint32_t)minimum);

      if (index >= table.symbolCount) {
        return false;
      }

      symbol = table.symbols[index];
      return true;
    }
  }

  return false;
}


// ============================================================
// RECEIVE AND EXTEND SIGNED MAGNITUDE
// ============================================================

bool DevEBox_JPEG::receiveExtend(
  File &file,
  uint8_t category,
  int16_t &value
) {

  if (category > 16) {
    return false;
  }

  if (category == 0) {
    value = 0;
    return true;
  }

  uint16_t bits;

  if (!readBits(file, category, bits)) {
    return false;
  }

  uint32_t threshold = 1UL << (category - 1);

  if (bits < threshold) {

    int32_t extended =
      (int32_t)bits - ((1UL << category) - 1);

    value = (int16_t)extended;

  }
  else {

    value = (int16_t)bits;
  }

  return true;
}


// ============================================================
// DECODE ONE 8x8 DCT BLOCK
// ============================================================

bool DevEBox_JPEG::decodeBlock(
  File &file,
  uint8_t componentIndex,
  int16_t coefficients[64]
) {

  if (componentIndex >= imageInfo_.componentCount) {
    return false;
  }

  if (!imageInfo_.scanFound) {
    return false;
  }

  memset(coefficients, 0, 64 * sizeof(int16_t));

  const Component &component =
    imageInfo_.components[componentIndex];

  const HuffmanTable &dcTable =
    imageInfo_.dcTables[component.dcTable];

  const HuffmanTable &acTable =
    imageInfo_.acTables[component.acTable];

  // --------------------------------------------------------
  // DC COEFFICIENT
  // --------------------------------------------------------

  uint8_t dcCategory;

  if (!decodeHuffmanSymbol(file, dcTable, dcCategory)) {
    return false;
  }

  // Baseline JPEG DC categories are 0 through 11.
  if (dcCategory > 11) {
    return false;
  }

  int16_t dcDifference;

  if (!receiveExtend(file, dcCategory, dcDifference)) {
    return false;
  }

  dcPredictors_[componentIndex] += dcDifference;

  if (dcPredictors_[componentIndex] < -32768 ||
      dcPredictors_[componentIndex] > 32767) {
    return false;
  }

  coefficients[0] = (int16_t)dcPredictors_[componentIndex];


  // --------------------------------------------------------
  // AC COEFFICIENTS
  // --------------------------------------------------------

  uint8_t position = 1;

  while (position < 64) {

    uint8_t symbol;

    if (!decodeHuffmanSymbol(file, acTable, symbol)) {
      return false;
    }

    uint8_t run = symbol >> 4;
    uint8_t size = symbol & 0x0F;

    // End of Block: remaining coefficients are zero.
    if (run == 0 && size == 0) {
      break;
    }

    // Zero Run Length: skip 16 zero coefficients.
    if (run == 15 && size == 0) {

      if (position + 16 > 64) {
        return false;
      }

      position += 16;
      continue;
    }

    // Baseline AC coefficient category is 1 through 10.
    if (size == 0 || size > 10) {
      return false;
    }

    if (position + run >= 64) {
      return false;
    }

    position += run;

    int16_t coefficient;

    if (!receiveExtend(file, size, coefficient)) {
      return false;
    }

    // Store in natural 8x8 coefficient order.
    coefficients[ZIGZAG[position]] = coefficient;

    position++;
  }

  return true;
}


// ============================================================
// DEQUANTIZE ONE 8x8 DCT BLOCK
// ============================================================

bool DevEBox_JPEG::dequantizeBlock(
  uint8_t componentIndex,
  const int16_t coefficients[64],
  int32_t dequantized[64]
) const {

  if (componentIndex >= imageInfo_.componentCount ||
      coefficients == nullptr ||
      dequantized == nullptr) {
    return false;
  }

  const Component &component =
    imageInfo_.components[componentIndex];

  const uint8_t tableID = component.quantizationTable;

  if (tableID >= MAX_TABLES ||
      !imageInfo_.quantizationTableDefined[tableID]) {
    return false;
  }

  const uint16_t *quantizationTable =
    imageInfo_.quantizationTables[tableID];

  // DQT values are stored in zigzag order, while the decoded
  // coefficients are stored in natural row-major order.
  for (uint8_t zigzagIndex = 0; zigzagIndex < 64; zigzagIndex++) {

    const uint8_t naturalIndex = ZIGZAG[zigzagIndex];
    const uint16_t quantizationValue =
      quantizationTable[zigzagIndex];

    // A zero quantization value is invalid for a JPEG DQT.
    if (quantizationValue == 0) {
      return false;
    }

    dequantized[naturalIndex] =
      (int32_t)coefficients[naturalIndex] *
      (int32_t)quantizationValue;
  }

  return true;
}



// ============================================================
// MCU GEOMETRY
// ============================================================

uint16_t DevEBox_JPEG::mcuWidth() const {

  if (!imageInfo_.baseline || imageInfo_.componentCount == 0) {
    return 0;
  }

  uint8_t maximumHorizontalSampling = 0;

  for (uint8_t i = 0; i < imageInfo_.componentCount; i++) {
    if (imageInfo_.components[i].horizontalSampling >
        maximumHorizontalSampling) {
      maximumHorizontalSampling =
        imageInfo_.components[i].horizontalSampling;
    }
  }

  return (uint16_t)maximumHorizontalSampling * 8;
}


uint16_t DevEBox_JPEG::mcuHeight() const {

  if (!imageInfo_.baseline || imageInfo_.componentCount == 0) {
    return 0;
  }

  uint8_t maximumVerticalSampling = 0;

  for (uint8_t i = 0; i < imageInfo_.componentCount; i++) {
    if (imageInfo_.components[i].verticalSampling >
        maximumVerticalSampling) {
      maximumVerticalSampling =
        imageInfo_.components[i].verticalSampling;
    }
  }

  return (uint16_t)maximumVerticalSampling * 8;
}


uint16_t DevEBox_JPEG::mcuColumns() const {

  const uint16_t width = mcuWidth();

  if (width == 0 || imageInfo_.width == 0) {
    return 0;
  }

  return (uint16_t)(
    ((uint32_t)imageInfo_.width + width - 1) / width
  );
}


uint16_t DevEBox_JPEG::mcuRows() const {

  const uint16_t height = mcuHeight();

  if (height == 0 || imageInfo_.height == 0) {
    return 0;
  }

  return (uint16_t)(
    ((uint32_t)imageInfo_.height + height - 1) / height
  );
}


// ============================================================
// CONSUME AND VALIDATE A RESTART MARKER
// ============================================================

bool DevEBox_JPEG::consumeRestartMarker(File &file) {

  // The remaining bits in the current entropy byte are padding bits.
  // A restart marker always begins at the next byte boundary.
  bitsRemaining_ = 0;
  bitBuffer_ = 0;

  uint8_t marker = 0;

  // readEntropyByte() may already have consumed the marker while
  // attempting to read beyond the entropy data. Reuse that marker
  // rather than reading past it.
  if (markerPending_) {
    marker = pendingMarker_;
    pendingMarker_ = 0;
    markerPending_ = false;
  }
  else {
    int value = file.read();

    if (value < 0 || (uint8_t)value != 0xFF) {
      return false;
    }

    // JPEG permits one or more 0xFF fill bytes before a marker code.
    do {
      value = file.read();
      if (value < 0) {
        return false;
      }
    } while ((uint8_t)value == 0xFF);

    marker = (uint8_t)value;

    // 0x00 is byte stuffing, not a restart marker.
    if (marker == 0x00) {
      return false;
    }
  }

  const uint8_t expected = 0xD0 + expectedRestartMarker_;

  if (marker != expected) {
    return false;
  }

  expectedRestartMarker_ = (expectedRestartMarker_ + 1) & 0x07;

  // DC differential prediction restarts independently for every
  // component at each restart interval.
  memset(dcPredictors_, 0, sizeof(dcPredictors_));

  return true;
}


// ============================================================
// DECODE ONE COMPLETE MCU
// ============================================================

bool DevEBox_JPEG::decodeMCU(File &file, MCU &mcu) {

  mcu.blockCount = 0;

  if (!file ||
      !imageInfo_.baseline ||
      !imageInfo_.scanFound ||
      imageInfo_.componentCount == 0 ||
      imageInfo_.scanComponentCount != imageInfo_.componentCount) {
    return false;
  }

  uint8_t maximumHorizontalSampling = 0;
  uint8_t maximumVerticalSampling = 0;
  uint8_t expectedBlockCount = 0;

  for (uint8_t i = 0; i < imageInfo_.componentCount; i++) {

    const Component &component = imageInfo_.components[i];

    if (component.horizontalSampling == 0 ||
        component.verticalSampling == 0) {
      return false;
    }

    if (component.horizontalSampling > maximumHorizontalSampling) {
      maximumHorizontalSampling = component.horizontalSampling;
    }

    if (component.verticalSampling > maximumVerticalSampling) {
      maximumVerticalSampling = component.verticalSampling;
    }

    expectedBlockCount +=
      component.horizontalSampling * component.verticalSampling;
  }

  if (expectedBlockCount == 0 ||
      expectedBlockCount > MAX_MCU_BLOCKS) {
    return false;
  }

  // Sampling factors must be integral divisors of the maximum
  // factors for this interleaved MCU layout.
  for (uint8_t i = 0; i < imageInfo_.componentCount; i++) {

    const Component &component = imageInfo_.components[i];

    if ((maximumHorizontalSampling %
         component.horizontalSampling) != 0 ||
        (maximumVerticalSampling %
         component.verticalSampling) != 0) {
      return false;
    }
  }

  // The entropy stream is ordered by SOS component order, not
  // necessarily by the component order in SOF0.
  for (uint8_t scanIndex = 0;
       scanIndex < imageInfo_.scanComponentCount;
       scanIndex++) {

    const uint8_t componentIndex =
      imageInfo_.scanComponentIndices[scanIndex];

    if (componentIndex >= imageInfo_.componentCount) {
      return false;
    }

    const Component &component =
      imageInfo_.components[componentIndex];

    // JPEG interleaved MCU block order:
    // component order from SOS, then vertical rows, then columns.
    for (uint8_t blockY = 0;
         blockY < component.verticalSampling;
         blockY++) {

      for (uint8_t blockX = 0;
           blockX < component.horizontalSampling;
           blockX++) {

        if (mcu.blockCount >= MAX_MCU_BLOCKS) {
          return false;
        }

        int16_t coefficients[64];
        int32_t dequantized[64];

        MCUBlock &outputBlock =
          mcu.blocks[mcu.blockCount];

        if (!decodeBlock(
              file,
              componentIndex,
              coefficients
            )) {
          return false;
        }

        if (!dequantizeBlock(
              componentIndex,
              coefficients,
              dequantized
            )) {
          return false;
        }

        if (!DevEBox_IDCT::transform(
              dequantized,
              outputBlock.pixels
            )) {
          return false;
        }

        outputBlock.componentIndex = componentIndex;
        outputBlock.blockX = blockX;
        outputBlock.blockY = blockY;

        mcu.blockCount++;
      }
    }
  }

  if (mcu.blockCount != expectedBlockCount) {
    return false;
  }

  decodedMCUs_++;

  // A restart marker follows every N MCUs, except when that boundary
  // is also the end of the complete scan.
  const uint16_t restartInterval =
    imageInfo_.restartIntervalDefined
      ? imageInfo_.restartInterval
      : 0;

  const uint32_t totalMCUs =
    (uint32_t)mcuColumns() * (uint32_t)mcuRows();

  if (restartInterval != 0 &&
      (decodedMCUs_ % restartInterval) == 0 &&
      decodedMCUs_ < totalMCUs) {

    if (!consumeRestartMarker(file)) {
      return false;
    }
  }

  return true;
}


// ============================================================
// PRINT JPEG HEADER INFORMATION
// ============================================================

void DevEBox_JPEG::printInfo(Stream &output) const {

  output.println();
  output.println("================================");
  output.println(" JPEG HEADER INFORMATION");
  output.println("================================");

  output.print("Width: ");
  output.println(imageInfo_.width);

  output.print("Height: ");
  output.println(imageInfo_.height);

  output.print("Precision: ");
  output.println(imageInfo_.precision);

  output.print("Components: ");
  output.println(imageInfo_.componentCount);

  output.println("-------------------------------");

  for (uint8_t i = 0; i < imageInfo_.componentCount; i++) {

    const Component &component = imageInfo_.components[i];

    output.print("Component ID: ");
    output.println(component.id);

    output.print("Horizontal sampling: ");
    output.println(component.horizontalSampling);

    output.print("Vertical sampling: ");
    output.println(component.verticalSampling);

    output.print("Quantization table: ");
    output.println(component.quantizationTable);

    output.print("DC Huffman table: ");
    output.println(component.dcTable);

    output.print("AC Huffman table: ");
    output.println(component.acTable);

    output.println("-------------------------------");
  }

  output.print("Restart interval: ");
  output.println(imageInfo_.restartInterval);

  output.print("Baseline frame: ");
  output.println(imageInfo_.baseline ? "YES" : "NO");

  output.print("Start of scan: ");
  output.println(imageInfo_.scanFound ? "FOUND" : "NOT FOUND");

  output.print("SOS component order: ");
  for (uint8_t i = 0; i < imageInfo_.scanComponentCount; i++) {
    output.print(imageInfo_.scanComponentIndices[i]);
    if (i + 1 < imageInfo_.scanComponentCount) {
      output.print(", ");
    }
  }
  output.println();

  output.println("================================");
}


// ============================================================
// ACCESS IMAGE INFORMATION
// ============================================================

const DevEBox_JPEG::ImageInfo &DevEBox_JPEG::info() const {
  return imageInfo_;
}
