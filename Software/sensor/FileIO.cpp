#include "FileIO.hpp"

#include "functions.h"

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

bool FileIO::writeRawData(SignalAnalyzer::Results const& analyzerResults, Config const& config)
{
    uint8_t dataSize = config.write8bit ? 1 : 4;

    // binsToProcess is a band; we should not ignore the start value here if we have IQ-data to let others do work
    // we should configure if we want ALL data or if we want to ignore certain parts.. To test future processing it
    // might be better to just have everything
    size_t binCount = config.audio.isIqMeasurement ? analyzerResults.setupData.binsToProcess.to * 2 + 2
                                                   : analyzerResults.setupData.binsToProcess.count();

    bool ok = true;
    if(hasToCreateNew(rawFile, config, rawFileCreation))
    {
        ok = openRawFile(binCount, config, dataSize);
        if(not ok)
            return false;
    }

    if(rawFile.write((byte*)&analyzerResults.timestamp, 4) != 4)
        return false;

    if(rawFile.write(&binCount, 4) != 4)
        return false;

    size_t startIndex = config.audio.isIqMeasurement
                            ? analyzerResults.setupData.iqOffset - analyzerResults.setupData.binsToProcess.to
                            : analyzerResults.setupData.binsToProcess.from;
    for(size_t i = startIndex; i < startIndex + binCount; i++)
    {
        if(config.write8bit)
        {
            ok = rawFile.write((uint8_t)std::abs(analyzerResults.spectrum[i]));
        }
        else
            ok = rawFile.write((byte*)&analyzerResults.spectrum[i], dataSize);

        if(not ok)
            return false;
    }

    static const uint32_t endMarker{~uint32_t{0}}; // all ones
    if(rawFile.write(&endMarker, 4) != 4)
        return false;

    rawFile.flush();
    return true;
}

bool FileIO::writeCsvMetricsData(SignalAnalyzer::Results const& analyzerResults, Config const& config)
{
    if(hasToCreateNew(csvMetricsFile, config, csvMetricsFileCreation))
    {
        bool ok = openCsvMetricsFile(config);
        if(not ok)
            return false;
    }

    csvMetricsFile.print(analyzerResults.timestamp);
    csvMetricsFile.print(", ");
    csvMetricsFile.print(analyzerResults.forward.detectedSpeed);
    csvMetricsFile.print(", ");
    csvMetricsFile.print(analyzerResults.reverse.detectedSpeed);
    csvMetricsFile.print(", ");
    csvMetricsFile.print(analyzerResults.forward.maxAmplitude);
    csvMetricsFile.print(", ");
    csvMetricsFile.print(analyzerResults.reverse.maxAmplitude);
    csvMetricsFile.print(", ");
    csvMetricsFile.print(analyzerResults.data.meanAmplitudeForPedestrians);
    csvMetricsFile.print(", ");
    csvMetricsFile.print(analyzerResults.data.meanAmplitudeForCars);
    csvMetricsFile.print(", ");
    csvMetricsFile.print(analyzerResults.data.meanAmplitudeForNoiseLevel);
    csvMetricsFile.print(", ");
    csvMetricsFile.print(analyzerResults.data.dynamicNoiseLevel);
    csvMetricsFile.print(", ");
    csvMetricsFile.print(analyzerResults.data.carTriggerSignal);

    csvMetricsFile.println("");
    csvMetricsFile.flush();

    return true;
}

bool FileIO::writeCsvCarData(SignalAnalyzer::Results const& analyzerResults, Config const& config)
{
    bool hasResult =
        analyzerResults.completedForwardDetection.has_value() || analyzerResults.completedReverseDetection.has_value();

    if(not hasResult)
        return true;

    if(hasToCreateNew(csvCarFile, config, csvCarFileCreation))
    {
        bool ok = openCsvCarFile(config);
        if(not ok)
            return false;
    }

    auto print = [this](std::optional<SignalAnalyzer::Results::ObjectDetection> const& optDetection) {
        if(not optDetection.has_value())
            return;

        auto const& detection = optDetection.value();

        csvCarFile.print(detection.timestamp);
        csvCarFile.print(", ");
        csvCarFile.print(detection.isForward ? "1" : "0");
        csvCarFile.print(", ");
        csvCarFile.print(detection.validSpeedCount);
        csvCarFile.print(", ");
        csvCarFile.print(detection.medianSpeed);
        csvCarFile.println("");

        Serial.print(detection.timestamp);
        Serial.print(", ");
        Serial.print(detection.isForward ? "1" : "0");
        Serial.print(", ");
        Serial.print(detection.validSpeedCount);
        Serial.print(", ");
        Serial.print(detection.medianSpeed);
        Serial.println("");
    };

    print(analyzerResults.completedForwardDetection);
    print(analyzerResults.completedReverseDetection);

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
    const char* teensyId = teensySerialNumberAsString();

    rawFile.write((byte*)&fileFormatVersion, 2);
    rawFile.write((byte*)&timestamp, 4);
    rawFile.write((byte*)&binCount, 2);
    rawFile.write((byte*)&dataSize, 1);
    rawFile.write((byte*)&config.analyzer.isIqMeasurement, 1);
    rawFile.write((byte*)&config.analyzer.signalSampleRate, 2);
    rawFile.write((byte*)teensyId, strlen(teensyId));
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
    csvMetricsFile.printf("// fileFormat=%d, teensyId=%s\n", fileFormatVersion, teensySerialNumberAsString());
    csvMetricsFile.print("timestamp, speed, speed_reverse, strength, strength_reverse, "
                         "meanAmplitudeForPedestrians, meanAmplitudeForCars, meanAmplitudeForNoiseLevel, ");

    csvMetricsFile.printf(
        "dynamic_noise_level_%d, car_trigger_signal_%d\n",
        config.analyzer.runningMeanHistoryN,
        config.analyzer.hannWindowN);

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

    csvCarFile.printf(
        "// fileFormat=%d, signalStrengthThreshold=%f, carSignalThreshold=%f, dynamicNoiseSmoothingFactor=%d, "
        "carTriggerSignalSmoothingFactor=%d, teensyId=%s\n",
        fileFormatVersion,
        config.analyzer.signalStrengthThreshold,
        config.analyzer.carSignalThreshold,
        config.analyzer.runningMeanHistoryN,
        config.analyzer.hannWindowN,
        teensySerialNumberAsString());

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

std::string trimmed(std::string const& input)
{
    std::string result = input;

    auto const isSpace = [](unsigned char ch) { return !std::isspace(ch); };

    result.erase(result.begin(), std::find_if(result.begin(), result.end(), isSpace));
    result.erase(std::find_if(result.rbegin(), result.rend(), isSpace).base(), result.end());

    return result;
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
                result[trimmed(key.c_str())] = trimmed(value.c_str());
        }

        configFile.close();
    }
    else
        Serial.println("(I) Did not found config file");

    return result;
}
