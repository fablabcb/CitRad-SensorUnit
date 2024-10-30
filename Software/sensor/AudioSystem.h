#ifndef AUDIOSYSTEM_H
#define AUDIOSYSTEM_H

#include <Audio.h>
#include <AudioStream_F32.h>
#include <OpenAudio_ArduinoLibrary.h>

#include <vector>

class AudioSystem
{
  public:
    static const size_t fftWidth = 1024;

  public:
    struct Config
    {
        const uint16_t sampleRate = 12000;             // Hz; sample rate of data acquisition
        const uint8_t audioInput = AUDIO_INPUT_LINEIN; // AUDIO_INPUT_LINEIN or AUDIO_INPUT_MIC
        const uint8_t lineInLevel = 15;                // only relevant if AUDIO_INPUT_LINEIN is used
        const bool isIqMeasurement = true;             // measure in both directions?
        float micGain = 1.0;                           // only relevant if AUDIO_INPUT_MIC is used

        // IQ calibration
        bool hasChanges = false;
        float alpha = 1.10;
        float psi = -0.04;

        Config& operator=(Config const& other);
    };

  public:
    AudioSystem();

    void setup(Config const& config);

    bool hasData();
    void extractSpectrum(std::vector<float>& spectrum);
    void updateIQ(Config const& config);
    float getPeak() const { return peak1.read(); }

  private:
    Config config;

    AudioAnalyzeFFT1024_IQ_F32 fft_IQ1024;
    mutable AudioAnalyzePeak_F32 peak1;
    AudioEffectGain_F32 I_gain; // iGain
    AudioMixer4_F32 Q_mixer;    // qMixer

    AudioInputI2S_F32 linein;
    AudioOutputI2S_F32 headphone;
    AudioControlSGTL5000 sgtl5000_1;
    AudioConnection_F32 patchCord1; // I_gain I input
    AudioConnection_F32 patchCord2; // FFT I input

    AudioConnection_F32 patchCord3;  // Q-mixer I input
    AudioConnection_F32 patchCord3b; // Q-mixer Q input
    AudioConnection_F32 patchCord4;  // FFT Q input

    AudioConnection_F32 patchCord5;
    AudioConnection_F32 patchCord6;
    AudioConnection_F32 patchCord7;

    float speedConversion; // conversion from Hz to m/s
};

#endif
