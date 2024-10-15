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

    // binsToProcess is a band; we ignore the start value if we have IQ-data
    size_t binCount = config.audio.isIqMeasurement ? audioResults.setupData.binsToProcess.to * 2 + 1
                                                   : audioResults.setupData.binsToProcess.count();

    bool ok = true;
    if(hasToCreateNew(rawFile, config, rawFileCreation))
    {
        ok = openRawFile(binCount, config, dataSize);
        if(not ok)
            return false;
    }

    if(rawFile.write((byte*)&audioResults.timestamp, 4) != 4)
        return false;

    if(rawFile.write(&binCount, 4) != 4)
        return false;

    size_t startIndex = config.audio.isIqMeasurement
                            ? audioResults.setupData.iqOffset - 1 - audioResults.setupData.binsToProcess.to
                            : audioResults.setupData.binsToProcess.from;
    for(int i = startIndex; i < startIndex + binCount; i++)
    {
        if(config.write8bit)
            ok = rawFile.write((uint8_t)audioResults.spectrum[i]);
        else
            ok = rawFile.write((byte*)&audioResults.spectrum[i], dataSize);

        if(not ok)
            return false;
    }

    static const uint32_t endMarker = ~0; // all ones
    if(rawFile.write(&endMarker, 4) != 4)
        return false;

    rawFile.flush();
    return true;
}

bool FileIO::writeCsvMetricsData(AudioSystem::Results const& audioResults, Config const& config)
{
    if(hasToCreateNew(csvMetricsFile, config, csvMetricsFileCreation))
    {
        bool ok = openCsvMetricsFile(config);
        if(not ok)
            return false;
    }

    csvMetricsFile.print(audioResults.timestamp);
    csvMetricsFile.print(", ");
    csvMetricsFile.print(audioResults.forward.detectedSpeed);
    csvMetricsFile.print(", ");
    csvMetricsFile.print(audioResults.reverse.detectedSpeed);
    csvMetricsFile.print(", ");
    csvMetricsFile.print(audioResults.forward.maxAmplitude);
    csvMetricsFile.print(", ");
    csvMetricsFile.print(audioResults.reverse.maxAmplitude);
    csvMetricsFile.print(", ");
    csvMetricsFile.print(audioResults.forward.binsWithSignal);
    csvMetricsFile.print(", ");
    csvMetricsFile.print(audioResults.reverse.binsWithSignal);
    csvMetricsFile.print(", ");
    csvMetricsFile.print(audioResults.data.meanAmplitudeForPedestrians);
    csvMetricsFile.print(", ");
    csvMetricsFile.print(audioResults.data.meanAmplitudeForCars);
    csvMetricsFile.print(", ");
    csvMetricsFile.print(audioResults.data.meanAmplitudeForNoiseLevel);
    csvMetricsFile.print(", ");
    csvMetricsFile.print(audioResults.data.dynamicNoiseLevel);
    csvMetricsFile.print(", ");
    csvMetricsFile.print(audioResults.data.carTriggerSignal);

    csvMetricsFile.println("");
    csvMetricsFile.flush();

    return true;
}

bool FileIO::writeCsvCarData(AudioSystem::Results const& audioResults, Config const& config)
{
    bool hasResult =
        audioResults.completedForwardDetection.has_value() || audioResults.completedReverseDetection.has_value();

    if(not hasResult)
        return true;

    if(hasToCreateNew(csvCarFile, config, csvCarFileCreation))
    {
        bool ok = openCsvCarFile(config);
        if(not ok)
            return false;
    }

    auto print = [this](std::optional<AudioSystem::Results::ObjectDetection> const& optDetection) {
        if(not optDetection.has_value())
            return;

        auto const& detection = optDetection.value();

        csvCarFile.print(detection.timestamp);
        csvCarFile.print(", ");
        csvCarFile.print(detection.isForward ? "1" : "0");
        csvCarFile.print(", ");
        csvCarFile.print(detection.sampleCount);
        csvCarFile.print(", ");
        csvCarFile.print(detection.medianSpeed);
        csvCarFile.println("");

        Serial.print(detection.timestamp);
        Serial.print(", ");
        Serial.print(detection.isForward ? "1" : "0");
        Serial.print(", ");
        Serial.print(detection.sampleCount);
        Serial.print(", ");
        Serial.print(detection.medianSpeed);
        Serial.println("");
    };

    print(audioResults.completedForwardDetection);
    print(audioResults.completedReverseDetection);

    csvCarFile.flush();

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
    rawFile.write((byte*)&config.audio.isIqMeasurement, 1);
    rawFile.write((byte*)&config.audio.sampleRate, 2);
    rawFile.flush();

    rawFileCreation = std::chrono::steady_clock::now();

    return true;
}

bool FileIO::openCsvMetricsFile(Config const& config)
{
    if(csvMetricsFile)
    {
        csvMetricsFile.flush();
        csvMetricsFile.close();
    }

    char filePattern[30];
    sprintf(filePattern, "metrics_%04d-%02d-%02d_%02d-%02d-%02d", year(), month(), day(), hour(), minute(), second());
    const String baseName = String(config.filePrefix.c_str()) + filePattern;

    openNewUniqueFile(csvMetricsFile, baseName, "csv");
    if(not csvMetricsFile)
    {
        Serial.println("(E) Failed to open new metrics csv file");
        return false;
    }
    csvMetricsFile.print("timestamp, speed, speed_reverse, strength, strength_reverse, "
                         "binsWithSignal, binsWithSignal_reverse,"
                         "meanAmplitudeForPedestrians, meanAmplitudeForCars, meanAmplitudeForNoiseLevel, ");

    csvMetricsFile.print(", dynamic_noise_level_");
    csvMetricsFile.print(config.audio.runningMeanHistoryN);
    csvMetricsFile.print(", car_trigger_signal_");
    csvMetricsFile.print(config.audio.hannWindowN);
    csvMetricsFile.println("");

    csvMetricsFile.flush();

    csvMetricsFileCreation = std::chrono::steady_clock::now();

    return true;
}

bool FileIO::openCsvCarFile(Config const& config)
{
    if(csvCarFile)
    {
        csvCarFile.flush();
        csvCarFile.close();
    }

    char filePattern[30];
    sprintf(filePattern, "cars_%04d-%02d-%02d_%02d-%02d-%02d", year(), month(), day(), hour(), minute(), second());
    const String baseName = String(config.filePrefix.c_str()) + filePattern;

    openNewUniqueFile(csvCarFile, baseName, "csv");
    if(not csvCarFile)
    {
        Serial.println("(E) Failed to open new car csv file");
        return false;
    }
    csvCarFile.println("timestamp, isForward, sampleCount, medianSpeed");
    csvCarFile.flush();

    csvCarFileCreation = std::chrono::steady_clock::now();

    return true;
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
