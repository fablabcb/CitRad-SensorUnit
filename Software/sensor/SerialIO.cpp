#include "SerialIO.hpp"

#include <SerialFlash.h>
#include <TimeLib.h>

void SerialIO::setup()
{
    // wait max 5s for serial connection

    Serial.begin(9600);
    int counter = 5;
    while(counter > 0 && !Serial)
    {
        delay(1000);
        counter--;
    }
    Serial.println("Hello Citizen Radar");
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

void SerialIO::sendOutput(AudioSystem::Results const& audioResults, AudioSystem const& audio, Config const& config)
{
    // send data via serial port - this is tied to the FFT_visualisation-pde java code
    auto const micGain = static_cast<int8_t>(config.audio.micGain);
    Serial.write((byte*)&micGain, 1);
    Serial.write((byte*)&audioResults.forward.maxFrequencyIdx, 2);

    // highest peak-to-peak distance of the signal (if >= 1 clipping occurs)
    float const peak = audio.getPeak();
    Serial.write((byte*)&peak, 4);

    Serial.write((byte*)&audioResults.setupData.numberOfFftBins, 2);

    for(size_t i = audioResults.setupData.minBinIndex; i < audioResults.setupData.maxBinIndex; i++)
        Serial.write((byte*)&(audioResults.noiseFloorDistance[i]), 4);
}
