#ifndef SERISALIO_HPP
#define SERISALIO_HPP

#include "AudioSystem.h"
#include "Config.hpp"
#include "SignalAnalyzer.hpp"

/**
 * @brief The SerialIO class provides means to communicate with the FFT_visualisation pde java code program
 */
class SerialIO
{
  public:
    void setup();

    static void printDigits(int digits);

    void processInputs(AudioSystem::Config& config, bool& sendOutput);
    void sendOutput(SignalAnalyzer::Results const& results, float audioPeak, Config const& config);
};

#endif
