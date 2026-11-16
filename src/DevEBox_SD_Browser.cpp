
#include "DevEBox_SD_Browser.h"

#include <string.h>

// ============================================================
// CONSTRUCTOR
// ============================================================

DevEBox_SD_Browser::DevEBox_SD_Browser(SdFat32 &sd)
  : sd_(sd) {

  strcpy(currentPath_, "/");
}

// ============================================================
// INITIALIZATION
// ============================================================

bool DevEBox_SD_Browser::begin() {

  strcpy(currentPath_, "/");

  File32 root = sd_.open("/", O_RDONLY);

  if (!root || !root.isDir()) {
    root.close();
    return false;
  }

  root.close();

  return true;
}

// ============================================================
// CURRENT DIRECTORY
// ============================================================

const char *DevEBox_SD_Browser::currentPath() const {

  return currentPath_;
}

// ============================================================
// BUILD ABSOLUTE PATH
// ============================================================

bool DevEBox_SD_Browser::buildPath(
  const char *name,
  char *destination,
  size_t destinationSize
) const {

  if (name == nullptr || name[0] == '\0') {
    return false;
  }

  int written;

  // Absolute path.
  if (name[0] == '/') {

    written = snprintf(
      destination,
      destinationSize,
      "%s",
      name
    );
  }

  // Relative path from root.
  else if (strcmp(currentPath_, "/") == 0) {

    written = snprintf(
      destination,
      destinationSize,
      "/%s",
      name
    );
  }

  // Relative path from current directory.
  else {

    written = snprintf(
      destination,
      destinationSize,
      "%s/%s",
      currentPath_,
      name
    );
  }

  return written >= 0 &&
         (size_t)written < destinationSize;
}

// ============================================================
// DIRECTORY LISTING
// ============================================================

void DevEBox_SD_Browser::listDirectory(Stream &output) {

  output.println();
  output.println("================================");
  output.println("DIRECTORY CONTENTS");
  output.println("================================");

  output.print("Path: ");
  output.println(currentPath_);

  File32 directory = sd_.open(
    currentPath_,
    O_RDONLY
  );

  if (!directory || !directory.isDir()) {

    output.println("ERROR: Cannot open directory.");

    directory.close();
    return;
  }

  uint16_t entryCount = 0;

  File32 entry;

  while (entry.openNext(&directory, O_RDONLY)) {

    char filename[MAX_FILENAME_LENGTH];

    filename[0] = '\0';

    entry.getName(
      filename,
      sizeof(filename)
    );

    const bool isDirectory = entry.isDir();

    output.print(isDirectory ? "[DIR]  " : "[FILE] ");
    output.print(filename);

    if (!isDirectory) {

      output.print("  (");
      output.print(entry.fileSize());
      output.print(" bytes)");
    }

    output.println();

    entryCount++;

    entry.close();
  }

  directory.close();

  output.println("--------------------------------");

  output.print("Total entries: ");
  output.println(entryCount);

  output.println("================================");
}

// ============================================================
// CHANGE DIRECTORY
// ============================================================

bool DevEBox_SD_Browser::changeDirectory(
  const char *directory,
  Stream &output
) {

  char newPath[MAX_PATH_LENGTH];

  if (!buildPath(
        directory,
        newPath,
        sizeof(newPath)
      )) {

    output.println("ERROR: Invalid path.");
    return false;
  }

  File32 target = sd_.open(
    newPath,
    O_RDONLY
  );

  if (!target || !target.isDir()) {

    output.println("ERROR: Directory not found.");

    target.close();
    return false;
  }

  target.close();

  strcpy(currentPath_, newPath);

  output.print("Entered: ");
  output.println(currentPath_);

  return true;
}

// ============================================================
// PARENT DIRECTORY
// ============================================================

bool DevEBox_SD_Browser::goUp(Stream &output) {

  if (strcmp(currentPath_, "/") == 0) {

    output.println("Already at root directory.");
    return false;
  }

  char *lastSlash = strrchr(currentPath_, '/');

  if (lastSlash == nullptr) {

    strcpy(currentPath_, "/");
  }

  else if (lastSlash == currentPath_) {

    strcpy(currentPath_, "/");
  }

  else {

    *lastSlash = '\0';
  }

  output.print("Current directory: ");
  output.println(currentPath_);

  return true;
}

// ============================================================
// OPEN FILE
// ============================================================

File32 DevEBox_SD_Browser::openFile(
  const char *filename,
  Stream &output
) {

  char filePath[MAX_PATH_LENGTH];

  if (!buildPath(
        filename,
        filePath,
        sizeof(filePath)
      )) {

    output.println("ERROR: Invalid filename.");
    return File32();
  }

  File32 file = sd_.open(
    filePath,
    O_RDONLY
  );

  if (!file) {

    output.println("ERROR: File not found.");
    return File32();
  }

  if (file.isDir()) {

    output.println("ERROR: This is a directory, not a file.");

    file.close();
    return File32();
  }

  output.print("Opening: ");
  output.println(filePath);

  return file;
}
