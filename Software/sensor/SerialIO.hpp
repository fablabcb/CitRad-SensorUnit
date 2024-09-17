#ifndef SERISALIO_HPP
#define SERISALIO_HPP

#include "AudioSystem.h"
#include "Config.h"

/**
 * @brief The SerialIO class provides means to communicate with the FFT_visualisation pde java code program
 */
class SerialIO
{
  public:
    static void printDigits(int digits);

    void processInputs(AudioSystem::Config& config, bool& sendOutput);
    void sendOutput(AudioSystem::Results const& results, AudioSystem const& audio, Config const& config);
};

#endif
