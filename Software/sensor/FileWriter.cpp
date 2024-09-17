#include "FileWriter.hpp"

#include <SPI.h>
#include <TimeLib.h> // year/month/etc

void FileWriter::writeRawHeader(size_t const binCount, bool iqMeasurement, uint16_t sampleRate)
{
    sprintf(rawFileName, "rawdata_%0d-%0d-%0d_%0d-%0d-%0d.BIN", year(), month(), day(), hour(), minute(), second());

    time_t timestamp = Teensy3Clock.get();

    rawFile = SD.open(rawFileName, FILE_WRITE);
    rawFile.write((byte*)&fileFormatVersion, 2);
    rawFile.write((byte*)&timestamp, 4);
    rawFile.write((byte*)&binCount, 2);
    rawFile.write((byte*)&iqMeasurement, 1);
    rawFile.write((byte*)&sampleRate, 2);
    rawFile.flush();
}

void FileWriter::writeRawData(AudioSystem::Results const& audioResults, bool write8bit)
{
    rawFile.write((byte*)&audioResults.timestamp, 4);

    for(int i = audioResults.minBinIndex; i < audioResults.maxBinIndex; i++)
        if(write8bit)
            rawFile.write((uint8_t)-audioResults.spectrum[i]);
        else
            rawFile.write((byte*)&audioResults.spectrum[i], 4);

    rawFile.flush();
}

void FileWriter::writeCsvHeader()
{
    sprintf(csvFileName, "metrics_%0d-%0d-%0d_%0d-%0d-%0d.csv", year(), month(), day(), hour(), minute(), second());

    csvFile = SD.open(csvFileName, FILE_WRITE);
    csvFile.println("timestamp, speed, speed_reverse, strength, strength_reverse, "
                    "mean_amplitude, mean_amplitude_reverse, bins_with_signal, "
                    "bins_with_signal_reverse, pedestrian_mean_amplitude");
    csvFile.flush();
}

void FileWriter::writeCsvData(AudioSystem::Results const& audioResults)
{
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

void FileWriter::setup()
{
    // Configure SPI
    SPI.setMOSI(SDCARD_MOSI_PIN);
    SPI.setSCK(SDCARD_SCK_PIN);
}

bool FileWriter::canWriteToSd() const
{
    return SD.begin(SDCARD_CS_PIN);
}
