#include "SerialIO.hpp"

#include "functions.h"

#include <SerialFlash.h>
#include <TimeLib.h>

void SerialIO::setup()
{
    // wait max 5s for serial connection

    Serial.begin(9600);
    SerialUSB1.begin(9600);
    int counter = 5;
    while(counter > 0 && !Serial)
    {
        delay(1000);
        counter--;
    }
}

void SerialIO::printDigits(int digits)
{
    // utility function for digital clock display: prints preceding colon and leading 0
    Serial.print(":");
    if(digits < 10)
        Serial.print('0');
    Serial.print(digits);
}

void SerialIO::processInputs(AudioSystem::Config& config, bool& sendOutput)
{
    // control input from serial
    while(Serial.available())
    {
        int8_t input = Serial.read();

        if(input == 100) // 'd' for data
            sendOutput = true;

        if((input == 0) & (config.micGain > 0.001))
        { // - key
            config.micGain -= 0.01;
        }
        if((input == 1) & (config.micGain < 10))
        { // + key
            config.micGain += 0.01;
        }

        if(input == 111)
        { // o
            config.alpha += 0.01;
        }
        if(input == 108)
        { // l
            config.alpha -= 0.01;
        }
        if(input == 105)
        { // i
            config.psi += 0.01;
        }
        if(input == 107)
        { // k
            config.psi -= 0.01;
        }

        if(input == 111 || input == 108 || input == 105 || input == 107)
        {
            Serial.print("alpha = ");
            Serial.println(config.alpha);
            Serial.print("psi = ");
            Serial.println(config.psi);

            config.hasChanges = true;
        }

        if(input == 84)
        {
            unsigned long externalTime = Serial.parseInt();

            // check if we got a somewhat recent time (greater than March 20 2024)
            unsigned long const minimalTime = 1710930219;
            if(externalTime >= minimalTime)
            {
                setTime(externalTime);          // Sync Arduino clock to the time received on the serial port
                Teensy3Clock.set(externalTime); // set Teensy RTC
            }

            Serial.print("Time set to: ");
            Serial.print(year());
            Serial.print("-");
            Serial.print(month());
            Serial.print("-");
            Serial.print(day());
            Serial.print(" ");
            Serial.print(hour());
            printDigits(minute());
            printDigits(second());
            Serial.println();
        }
    }
}

void SerialIO::sendOutput(SignalAnalyzer::Results const& analyzerResults, float audioPeak, Config const& config)
{
    // send data via serial port - this is tied to the FFT_visualisation-pde java code
    auto const micGain = static_cast<int8_t>(config.audio.micGain);
    SerialUSB1.write((byte*)&micGain, 1);
    // this has been removed on the sensor side; we just send 0 instead
    // was: Serial.write((byte*)&analyzerResults.forward.maxAmplitudeIdx, 2);
    uint16_t fake = 0;
    SerialUSB1.write((byte*)&fake, 2);

    // highest peak-to-peak distance of the signal (if >= 1 clipping occurs)
    SerialUSB1.write((byte*)&audioPeak, 4);

    uint16_t binCount = analyzerResults.setupData.binsToProcess.count();
    SerialUSB1.write((byte*)&binCount, 2);

    for(size_t i = analyzerResults.setupData.binsToProcess.from; i <= analyzerResults.setupData.binsToProcess.to; i++)
        SerialUSB1.write((byte*)&(analyzerResults.noiseFloorDistance[analyzerResults.setupData.iqOffset + i + 1]), 4);
}

void SerialIO::sendData(char const* data, size_t size, size_t timestamp, size_t counter)
{
    static const uint32_t startMarker{~uint32_t{0}}; // all ones

    unsigned char header[16];
    memcpy(header + 0, &startMarker, 4);
    memcpy(header + 4, &counter, 4);
    memcpy(header + 8, &timestamp, 4);
    memcpy(header + 12, &size, 4);

    SerialUSB1.write((byte*)header, 16);
    SerialUSB1.send_now();
    SerialUSB1.write((byte*)data, size);
}

void SerialIO::onLoop(Config& config)
{
    bool hasConnection = Serial.dtr();
    if(hasConnection)
    {
        if(not hadConnection)
        {
            Serial.printf("Hello Citizen Radar Monitor - I am %s\n", teensySerialNumberAsString());
        }
    }
    hadConnection = hasConnection;
}
