
#pragma once

#include <Arduino.h>
#include <SD.h>

class DevEBox_SD_Browser {

public:

  static const size_t MAX_PATH_LENGTH = 128;

  DevEBox_SD_Browser();

  bool begin();

  void listDirectory(Stream &output);

  bool changeDirectory(const char *directory, Stream &output);

  bool goUp(Stream &output);

  const char *currentPath() const;

  File openFile(const char *filename, Stream &output);

private:

  char currentPath_[MAX_PATH_LENGTH];

  bool buildPath(
    const char *name,
    char *destination,
    size_t destinationSize
  ) const;
};
