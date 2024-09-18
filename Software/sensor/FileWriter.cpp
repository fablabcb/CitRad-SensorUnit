#include "FileWriter.hpp"

#include <SPI.h>
#include <TimeLib.h> // year/month/etc

#include <string>

bool hasToCreateNew(File& file, Config const& config, std::chrono::steady_clock::time_point const& oldCreationTime)
{
    if(not file)
        return true;

    if(not config.splitLargeFiles)
        return false;

    using namespace std::chrono;
    auto const now = steady_clock::now();
    return duration_cast<seconds>(now - oldCreationTime).count() >= config.maxSecondsPerFile;
}

void FileWriter::writeRawData(AudioSystem::Results const& audioResults, bool write8bit, Config const& config)
{
    if(hasToCreateNew(rawFile, config, rawFileCreation))
        openRawFile(audioResults.numberOfFftBins, config);

    rawFile.write((byte*)&audioResults.timestamp, 4);

    for(int i = audioResults.minBinIndex; i < audioResults.maxBinIndex; i++)
        if(write8bit)
            rawFile.write((uint8_t)-audioResults.spectrum[i]);
        else
            rawFile.write((byte*)&audioResults.spectrum[i], 4);

    rawFile.flush();
}

void FileWriter::writeCsvData(AudioSystem::Results const& audioResults, Config const& config)
{
    if(hasToCreateNew(csvFile, config, csvFileCreation))
        openCsvFile(config);

    csvFile.print(audioResults.timestamp);
    csvFile.print(", ");
    csvFile.print(audioResults.detected_speed);
    csvFile.print(", ");
    csvFile.print(audioResults.detected_speed_reverse);
    csvFile.print(", ");
    csvFile.print(audioResults.amplitudeMax);
    csvFile.print(", ");
    csvFile.print(audioResults.amplitudeMaxReverse);
    csvFile.print(", ");
    csvFile.print(audioResults.mean_amplitude);
    csvFile.print(", ");
    csvFile.print(audioResults.mean_amplitude_reverse);
    csvFile.print(", ");
    csvFile.print(audioResults.bins_with_signal);
    csvFile.print(", ");
    csvFile.print(audioResults.bins_with_signal_reverse);
    csvFile.print(", ");
    csvFile.println(audioResults.pedestrian_amplitude);
    csvFile.flush();
}

void FileWriter::openRawFile(size_t const binCount, Config const& config)
{
    if(rawFile)
    {
        rawFile.flush();
        rawFile.close();
    }

    char filePattern[30];
    sprintf(filePattern, "%04d-%02d-%02d_%02d-%02d-%02d.bin", year(), month(), day(), hour(), minute(), second());
    const String fileName = config.filePrefix + filePattern;

    Serial.println("Creating new file: " + fileName);

    time_t timestamp = Teensy3Clock.get();

    rawFile = SD.open(fileName.c_str(), FILE_WRITE);
    rawFile.write((byte*)&fileFormatVersion, 2);
    rawFile.write((byte*)&timestamp, 4);
    rawFile.write((byte*)&binCount, 2);
    rawFile.write((byte*)&config.audio.iq_measurement, 1);
    rawFile.write((byte*)&config.audio.sample_rate, 2);
    rawFile.flush();

    rawFileCreation = std::chrono::steady_clock::now();
}

void FileWriter::openCsvFile(Config const& config)
{
    if(csvFile)
    {
        csvFile.flush();
        csvFile.close();
    }

    char filePattern[30];
    sprintf(filePattern, "%04d-%02d-%02d_%02d-%02d-%02d.csv", year(), month(), day(), hour(), minute(), second());
    const String fileName = config.filePrefix + filePattern;

    Serial.println("Creating new file: " + fileName);

    csvFile = SD.open(fileName.c_str(), FILE_WRITE);
    csvFile.println("timestamp, speed, speed_reverse, strength, strength_reverse, "
                    "mean_amplitude, mean_amplitude_reverse, bins_with_signal, "
                    "bins_with_signal_reverse, pedestrian_mean_amplitude");
    csvFile.flush();

    csvFileCreation = std::chrono::steady_clock::now();
}

void FileWriter::setupSpi()
{
    // Configure SPI
    SPI.setMOSI(SDCARD_MOSI_PIN);
    SPI.setSCK(SDCARD_SCK_PIN);
}

bool FileWriter::setupSdCard()
{
    return SD.begin(SDCARD_CS_PIN) == 1;
}
