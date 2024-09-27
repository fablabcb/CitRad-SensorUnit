#ifndef AUDIOSYSTEM_H
#define AUDIOSYSTEM_H

#include <Audio.h>
#include <AudioStream_F32.h>
#include <OpenAudio_ArduinoLibrary.h>

#include <array>
#include <cstddef>

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

        const float noiseFloorDistance_threshold = 8; // dB; distance of "proper signal" to noise floor
        float micGain = 1.0;                          // only relevant if AUDIO_INPUT_MIC is used

        // IQ calibration
        bool hasChanges = false;
        float alpha = 1.10;
        float psi = -0.04;

        Config& operator=(Config const& other);
    };

    // data derived from config
    struct SetupData
    {
        uint16_t maxBinIndex;
        uint16_t minBinIndex;
        uint16_t pedestriansBinMax; // maximum bin index to even consider to contain pedestrian data
        uint16_t numberOfFftBins;
        uint16_t iqOffset; // offset used for IQ calculation
    };

    struct Results
    {
        struct Data
        {
            float amplitudeMax = -9999;   // highest signal in spectrum
            uint16_t maxFrequencyIdx = 0; // index of highest signal in spectrum
            float detectedSpeed = 0.0;    // speed in m/s based on peak frequency
            float meanAmplitude = 0.0;    // mean amplitude in spectrum used to detect cars passing by the sensor
            uint8_t binsWithSignal = 0;   // how many bins have signal over the noise threshold?
        };

        std::array<float, fftWidth> noiseFloorDistance;
        std::array<float, fftWidth> spectrum;         // spectral data
        std::array<float, fftWidth> spectrumSmoothed; // smoothed spectral data for noise floor

        Data forward; // result data for forward direction
        Data reverse; // result data for backward direction

        SetupData setupData;
        float pedestrianAmplitude; // used to detect the presence of a pedestrians

        unsigned long timestamp; // ms of how long the sensor has been running

        void process(float* data, float noiseFloorDistanceThreshold, float speedConversion);
    };

  public:
    AudioSystem();

    void setup(Config const& config, float maxPedestrianSpeed, float sendMaxSpeed);
    void processData(Results& results);

    bool hasData();
    void updateIQ(Config const& config);

    float getPeak() const { return peak1.read(); }

  private:
    Config config;
    SetupData setupData;

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
