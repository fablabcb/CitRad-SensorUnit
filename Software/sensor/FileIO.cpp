#include "FileIO.hpp"

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

void FileIO::writeRawData(AudioSystem::Results const& audioResults, bool write8bit, Config const& config)
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

void FileIO::writeCsvData(AudioSystem::Results const& audioResults, Config const& config)
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

void FileIO::openRawFile(size_t const binCount, Config const& config)
{
    if(rawFile)
    {
        rawFile.flush();
        rawFile.close();
    }

    char filePattern[30];
    sprintf(filePattern, "%04d-%02d-%02d_%02d-%02d-%02d.bin", year(), month(), day(), hour(), minute(), second());
    const String fileName = String(config.filePrefix.c_str()) + filePattern;

    Serial.println("(D) Creating new file: " + fileName);

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

void FileIO::openCsvFile(Config const& config)
{
    if(csvFile)
    {
        csvFile.flush();
        csvFile.close();
    }

    char filePattern[30];
    sprintf(filePattern, "%04d-%02d-%02d_%02d-%02d-%02d.csv", year(), month(), day(), hour(), minute(), second());
    const String fileName = String(config.filePrefix.c_str()) + filePattern;

    Serial.println("(D) Creating new file: " + fileName);

    csvFile = SD.open(fileName.c_str(), FILE_WRITE);
    csvFile.println("timestamp, speed, speed_reverse, strength, strength_reverse, "
                    "mean_amplitude, mean_amplitude_reverse, bins_with_signal, "
                    "bins_with_signal_reverse, pedestrian_mean_amplitude");
    csvFile.flush();

    csvFileCreation = std::chrono::steady_clock::now();
}

void FileIO::setupSpi()
{
    // Configure SPI
    SPI.setMOSI(SDCARD_MOSI_PIN);
    SPI.setSCK(SDCARD_SCK_PIN);
}

bool FileIO::setupSdCard()
{
    return SD.begin(SDCARD_CS_PIN) == 1;
}

std::map<std::string, std::string> FileIO::readConfigFile() const
{
    std::map<std::string, std::string> result;

    File configFile = SD.open("config.txt", FILE_READ);
    if(configFile)
    {
        Serial.println("(I) Reading config file");

        String key;
        String value;
        while(configFile.available())
        {
            key = configFile.readStringUntil(':');
            value = configFile.readStringUntil('\n');

            if(key.length() > 0 && value.length() > 0)
                result[key.c_str()] = value.c_str();
        }

        configFile.close();
    }
    else
        Serial.println("(I) Did not found config file");

    return result;
}
