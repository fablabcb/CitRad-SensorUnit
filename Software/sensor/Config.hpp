#ifndef CONFIG_H
#define CONFIG_H

#include "AudioSystem.h"

#include <cstddef>
#include <map>
#include <string>

struct Config
{
    AudioSystem::Config audio;

    float maxPedestrianSpeed = 10.0; // m/s; speed under which signals are detected as pedestrians
    float maxSpeedToUse = 500;       // don't send (and store) spectral data higher than this speed

    bool writeDataToSdCard = true; // write data to SD card?
    bool write8bit = true;         // write data as 8bit binary (to save disk space)
    bool writeRawData = true;      // write raw spectral data to SD?
    bool writeCsvData = true;      // write calculated metrix to csv table?

    bool splitLargeFiles = true;     // if true, the raw and csv files will be split after each given timespan
    size_t maxSecondsPerFile = 3600; // used if splitLargeFiles is true
    std::string filePrefix;          // file name prefix (containing id and stuff)

    void process(std::map<std::string, std::string> const& map);

    Config()
        : filePrefix("test_unit_")
    {}
};

#endif
