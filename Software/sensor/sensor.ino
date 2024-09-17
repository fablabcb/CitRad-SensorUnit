#include "AudioSystem.h"
#include "Config.h"
#include "FileWriter.hpp"
#include "SerialIO.hpp"
#include "functions.h"

#include <SerialFlash.h>
#include <Wire.h>
#include <utility/imxrt_hw.h>

AudioSystem audio;
AudioSystem::Results audioResults; // global to optimize for speed
Config config;

FileWriter fileWriter;
SerialIO serialIO;

void setup()
{
    setSyncProvider(getTeensy3Time);
    setI2SFreq(config.audio.sample_rate);

    // TODO maybe move to audio
    pinMode(PIN_A3, OUTPUT); // A3=17, A8=22, A4=18
    digitalWrite(PIN_A4, LOW);

    audio.setup(config.audio, config.max_pedestrian_speed, config.send_max_speed);

    // TODO this seems to be a get&set to and from the same data point?
    // setTime(Teensy3Clock.get());
    // timestamp = now();
    {
        time_t timestamp = Teensy3Clock.get();
        setTime(timestamp);
    }

    fileWriter.setup();
    if(not fileWriter.canWriteToSd())
    {
        Serial.println("Unable to access the SD card");
    }
    else
    {
        if(config.writeRawData)
            fileWriter.writeRawHeader(
                audio.getNumberOfFftBins(),
                config.audio.iq_measurement,
                config.audio.sample_rate);

        if(config.writeCsvData)
            fileWriter.writeCsvHeader();
    }
}

void loop()
{
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

    // elapsed time since start of sensor in milliseconds
    audioResults.timestamp = millis();
    audio.processData(audioResults);

    if(config.writeDataToSdCard)
    {
        if(config.writeRawData)
            fileWriter.writeRawData(audioResults, config.write8bit);

        if(config.writeCsvData)
            fileWriter.writeCsvData(audioResults);

        // SerialUSB1.print("csv sd write time: ");
        // SerialUSB1.println(millis()-time_millis);
    }

    if(sendOutput)
        serialIO.sendOutput(audioResults, audio, config);
}
