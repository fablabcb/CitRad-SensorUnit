#ifndef CONFIG_H
#define CONFIG_H

#include "AudioSystem.h"
#include "SignalAnalyzer.hpp"

#include <cstddef>
#include <map>
#include <string>

struct Config
{
    AudioSystem::Config audio;
    SignalAnalyzer::Config analyzer;

    bool writeDataToSdCard = true;    // write data to SD card?
    bool write8bit = true;            // write data as 8bit binary (to save disk space)
    bool writeRawData = false;        // write raw spectral data to SD?
    bool writeCsvMetricsData = false; // write calculated metrics to csv table?
    bool writeCsvCarData = true;      // write car signals to csv table?

    bool splitLargeFiles = true;     // if true, the raw and csv files will be split after each given timespan
    size_t maxSecondsPerFile = 3600; // used if splitLargeFiles is true
    std::string filePrefix;          // file name prefix (containing id and stuff)

    void process(std::map<std::string, std::string> const& map);

    Config()
        : filePrefix("test_unit_")
    {}
};

#endif
