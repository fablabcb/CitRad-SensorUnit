#ifndef FILEWRITER_HPP
#define FILEWRITER_HPP

#include "AudioSystem.h"
#include "Config.h"

#include <SD.h>

#include <chrono>
#include <stdint.h>

class FileWriter
{
  public:
    void writeRawData(AudioSystem::Results const& audioResults, bool write8bit, Config const& config);
    void writeCsvData(AudioSystem::Results const& audioResults, Config const& config);

    void setupSpi();
    bool setupSdCard();

  private:
    void openRawFile(size_t const binCount, Config const& config);
    void openCsvFile(Config const& config);

  private:
    File rawFile;
    File csvFile;

    std::chrono::steady_clock::time_point rawFileCreation;
    std::chrono::steady_clock::time_point csvFileCreation;

  private:
    const int SDCARD_MOSI_PIN = 11; // Teensy 4 ignores this, uses pin 11
    const int SDCARD_SCK_PIN = 13;  // Teensy 4 ignores this, uses pin 13
    const int SDCARD_CS_PIN = 10;
    const uint16_t fileFormatVersion = 1;
};

#endif
