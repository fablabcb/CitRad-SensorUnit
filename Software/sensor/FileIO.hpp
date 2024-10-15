#ifndef FILEIO_HPP
#define FILEIO_HPP

#include "AudioSystem.h"
#include "Config.hpp"

#include <SD.h>

#include <chrono>
#include <cstddef>
#include <map>
#include <string>

class FileIO
{
  public:
    bool writeRawData(AudioSystem::Results const& audioResults, Config const& config);
    bool writeCsvMetricsData(AudioSystem::Results const& audioResults, Config const& config);
    bool writeCsvCarData(AudioSystem::Results const& audioResults, Config const& config);

    void setupSpi();
    bool setupSdCard();
    void closeSdCard();

    std::map<std::string, std::string> readConfigFile() const;

  private:
    bool openRawFile(uint16_t const binCount, Config const& config, uint8_t const& dataSize);
    bool openCsvMetricsFile(Config const& config);
    bool openCsvCarFile(Config const& config);

  private:
    File rawFile;
    File csvMetricsFile;
    File csvCarFile;

    std::chrono::steady_clock::time_point rawFileCreation;
    std::chrono::steady_clock::time_point csvMetricsFileCreation;
    std::chrono::steady_clock::time_point csvCarFileCreation;

  private:
    const int SDCARD_MOSI_PIN = 11; // Teensy 4 ignores this, uses pin 11
    const int SDCARD_SCK_PIN = 13;  // Teensy 4 ignores this, uses pin 13
    const int SDCARD_CS_PIN = 10;
    const uint16_t fileFormatVersion = 2;
};

#endif
