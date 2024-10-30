#include "AudioSystem.h"
#include "Config.hpp"
#include "FileIO.hpp"
#include "SerialIO.hpp"
#include "SignalAnalyzer.hpp"
#include "functions.h"

#include <SerialFlash.h>
#include <Wire.h>
#include <utility/imxrt_hw.h>

AudioSystem audio;
SignalAnalyzer signalAnalyzer;
SignalAnalyzer::Results results; // global to optimize for speed
Config config;

FileIO fileWriter;
SerialIO serialIO;

bool canWriteData = false;

void onSdCardActive()
{
    canWriteData = true;
    Serial.println("(I) SD card initialized");
    auto configFromFile = fileWriter.readConfigFile();
    config.process(configFromFile);
}

void setup()
{
    setSyncProvider(getTeensy3Time);
    setI2SFreq(config.audio.sampleRate);

    // TODO maybe move to audio
    pinMode(PIN_A3, OUTPUT); // A3=17, A8=22, A4=18
    digitalWrite(PIN_A4, LOW);

    // TODO this seems to be a get&set to and from the same data point?
    // setTime(Teensy3Clock.get());
    // timestamp = now();
    {
        time_t timestamp = Teensy3Clock.get();
        setTime(timestamp);
    }

    serialIO.setup();
    fileWriter.setupSpi();

    if(fileWriter.setupSdCard())
        onSdCardActive();
    else
        Serial.println("(W) Unable to access the SD card");

    audio.setup(config.audio);

    config.analyzer.signalSampleRate = config.audio.sampleRate;
    config.analyzer.fftWidth = AudioSystem::fftWidth;
    config.analyzer.isIqMeasurement = config.audio.isIqMeasurement;
    signalAnalyzer.setup(config.analyzer);
}

void loop()
{
    if(not canWriteData)
    {
        delay(1000);

        if(fileWriter.setupSdCard())
            onSdCardActive();

        return;
    }

    if(not audio.hasData())
    {
        delay(1);
        return;
    }

    bool sendOutput = false; // send output over serial - attention: this is only send once and needs to be preserved
    // and thus might be better put into global state or something

    serialIO.processInputs(config.audio, sendOutput);
    if(config.audio.hasChanges)
    {
        audio.updateIQ(config.audio);
        config.audio.hasChanges = false;
    }

    audio.extractSpectrum(results.spectrum);
    signalAnalyzer.processData(results);
    results.timestamp = millis();

    if(config.writeDataToSdCard)
    {
        bool ok = false;

        if(config.writeRawData)
        {
            ok = fileWriter.writeRawData(results, config);
            if(not ok)
            {
                Serial.println("(E) Failed to write raw data to SD card");
                canWriteData = false;
                // there does not seem to be a sane way to check if the SD card is back again - hope for the best
            }
        }

        if(config.writeCsvMetricsData)
        {
            ok = fileWriter.writeCsvMetricsData(results, config);
            if(not ok)
            {
                Serial.println("(E) Failed to write csv metrics data to SD card");
                canWriteData = false;
                // there does not seem to be a sane way to check if the SD card is back again - hope for the best
            }
        }

        if(config.writeCsvCarData)
        {
            ok = fileWriter.writeCsvCarData(results, config);
            if(not ok)
            {
                Serial.println("(E) Failed to write csv car data to SD card");
                canWriteData = false;
                // there does not seem to be a sane way to check if the SD card is back again - hope for the best
            }
        }

        // SerialUSB1.println(second());
        // SerialUSB1.print("csv sd write time: ");
    }

    if(sendOutput)
        serialIO.sendOutput(results, audio.getPeak(), config);
}
