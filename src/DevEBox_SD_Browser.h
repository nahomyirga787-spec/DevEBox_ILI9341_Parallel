
#pragma once

#include <Arduino.h>
#include <SdFat.h>

class DevEBox_SD_Browser {

public:

  static const size_t MAX_PATH_LENGTH = 128;
  static const size_t MAX_FILENAME_LENGTH = 64;

  explicit DevEBox_SD_Browser(SdFat32 &sd);

  bool begin();

  void listDirectory(Stream &output);

  bool changeDirectory(
    const char *directory,
    Stream &output
  );

  bool goUp(Stream &output);

  const char *currentPath() const;

  File32 openFile(
    const char *filename,
    Stream &output
  );

private:

  SdFat32 &sd_;

  char currentPath_[MAX_PATH_LENGTH];

  bool buildPath(
    const char *name,
    char *destination,
    size_t destinationSize
  ) const;
};
