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

void openNewUniqueFile(File& file, String const& baseName, String const& fileExtension)
{
    String fileName = baseName + "." + fileExtension;

    int suffix = 0;
    String newName = fileName;
    while(SD.exists(newName.c_str()))
    {
        suffix++;
        newName = baseName + "." + String(suffix) + "." + fileExtension;
    }

    Serial.println("(D) Creating new file: " + newName);

    file = SD.open(newName.c_str(), FILE_WRITE);
}

bool FileIO::writeRawData(AudioSystem::Results const& audioResults, Config const& config)
{
    uint8_t dataSize = config.write8bit ? 1 : 4;

    bool ok = true;
    if(hasToCreateNew(rawFile, config, rawFileCreation))
    {
        ok = openRawFile(audioResults.numberOfFftBins, config, dataSize);
        if(not ok)
            return false;
    }

    if(rawFile.write((byte*)&audioResults.timestamp, 4) != 4)
        return false;

    uint32_t count = audioResults.maxBinIndex - audioResults.minBinIndex;
    if(rawFile.write(&count, 4) != 4)
        return false;

    for(int i = audioResults.minBinIndex; i < audioResults.maxBinIndex; i++)
    {
        if(config.write8bit)
            ok = rawFile.write((uint8_t)audioResults.spectrum[i]);
        else
            ok = rawFile.write((byte*)&audioResults.spectrum[i], dataSize);

        if(not ok)
            return;
    }

    static const uint32_t endMarker = ~0; // all ones; will be NaN in float
    if(rawFile.write(&count, 4) != 4)
        return false;

    rawFile.flush();
    return true;
}

bool FileIO::writeCsvData(AudioSystem::Results const& audioResults, Config const& config)
{
    if(hasToCreateNew(csvFile, config, csvFileCreation))
    {
        bool ok = openCsvFile(config);
        if(not ok)
            return false;
    }

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

    return true;
}

bool FileIO::openRawFile(uint16_t const binCount, Config const& config, uint8_t const& dataSize)
{
    if(rawFile)
    {
        rawFile.flush();
        rawFile.close();
    }

    char filePattern[30];
    sprintf(filePattern, "%04d-%02d-%02d_%02d-%02d-%02d", year(), month(), day(), hour(), minute(), second());
    const String baseName = String(config.filePrefix.c_str()) + filePattern;

    openNewUniqueFile(rawFile, baseName, "bin");
    if(not rawFile)
    {
        Serial.println("(E) Failed to open new raw file");
        return false;
    }

    time_t timestamp = Teensy3Clock.get();

    rawFile.write((byte*)&fileFormatVersion, 2);
    rawFile.write((byte*)&timestamp, 4);
    rawFile.write((byte*)&binCount, 2);
    rawFile.write((byte*)&dataSize, 1);
    rawFile.write((byte*)&config.audio.iq_measurement, 1);
    rawFile.write((byte*)&config.audio.sample_rate, 2);
    rawFile.flush();

    rawFileCreation = std::chrono::steady_clock::now();
}

bool FileIO::openCsvFile(Config const& config)
{
    if(csvFile)
    {
        csvFile.flush();
        csvFile.close();
    }

    char filePattern[30];
    sprintf(filePattern, "%04d-%02d-%02d_%02d-%02d-%02d", year(), month(), day(), hour(), minute(), second());
    const String baseName = String(config.filePrefix.c_str()) + filePattern;

    openNewUniqueFile(csvFile, baseName, "csv");
    if(not csvFile)
    {
        Serial.println("(E) Failed to open new csv file");
        return false;
    }
    csvFile.println("timestamp, speed, speed_reverse, strength, strength_reverse, "
                    "mean_amplitude, mean_amplitude_reverse, bins_with_signal, "
                    "bins_with_signal_reverse, pedestrian_mean_amplitude");
    csvFile.flush();

    csvFileCreation = std::chrono::steady_clock::now();
}

void FileIO::setupSpi()
{
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
