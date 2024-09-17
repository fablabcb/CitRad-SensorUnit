#ifndef FILEWRITER_HPP
#define FILEWRITER_HPP

#include "AudioSystem.h"

#include <SD.h>

#include <stdint.h>

class FileWriter
{
  public:
    void writeRawHeader(size_t const binCount, bool iqMeasurement, uint16_t sampleRate);
    void writeRawData(AudioSystem::Results const& audioResults, bool write8bit);

    void writeCsvHeader();
    void writeCsvData(AudioSystem::Results const& audioResults);

    void setup();
    bool canWriteToSd() const;

  private:
    char rawFileName[30];
    char csvFileName[30];

    File rawFile;
    File csvFile;

  private:
    const int SDCARD_MOSI_PIN = 11; // Teensy 4 ignores this, uses pin 11
    const int SDCARD_SCK_PIN = 13;  // Teensy 4 ignores this, uses pin 13
    const int SDCARD_CS_PIN = 10;
    const uint16_t fileFormatVersion = 1;
};

#endif
