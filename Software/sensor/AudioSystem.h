#ifndef AUDIOSYSTEM_H
#define AUDIOSYSTEM_H

#include <Audio.h>
#include <AudioStream_F32.h>
#include <OpenAudio_ArduinoLibrary.h>

class AudioSystem
{
  public:
    struct Config
    {
        const unsigned int fftWidth = 1024;

        const uint16_t sample_rate = 12000;             // Hz; sample rate of data acquisition
        const uint8_t audio_input = AUDIO_INPUT_LINEIN; // AUDIO_INPUT_LINEIN or AUDIO_INPUT_MIC
        const uint8_t linein_level = 15;                // only relevant if AUDIO_INPUT_LINEIN is used
        const bool iq_measurement = true;               // measure in both directions?

        const float noise_floor_distance_threshold = 8; // dB; distance of "proper signal" to noise floor
        float mic_gain = 1.0;                           // only relevant if AUDIO_INPUT_MIC is used

        // IQ calibration
        bool hasChanges = false;
        float alpha = 1.10;
        float psi = -0.04;

        Config& operator=(Config const& other);
    };

    struct Results
    {
        // spectrum

        float noise_floor_distance[1024];
        float spectrum[1024];                // spectral data
        float spectrum_smoothed[1024] = {0}; // smoothed spectral data for noise floor

        float amplitudeMax;        // highest signal in spectrum
        float amplitudeMaxReverse; // highest signal in spectrum reverse direction

        uint16_t max_freq_Index;         // index of highest signal in spectrum
        uint16_t max_freq_Index_reverse; // index of highest signal in spectrum reverse direction

        uint16_t max_pedestrian_bin;
        float pedestrian_amplitude;   // used to detect the presence of a pedestrians
        float detected_speed;         // speed in m/s based on peak frequency
        float detected_speed_reverse; // speed in m/s based on peak frequency reverse direction

        float mean_amplitude; // mean amplitude in spectrum used to detect cars passing by the sensor
        float mean_amplitude_reverse;

        uint8_t bins_with_signal; // how many bins have signal over the noise threshold?
        uint8_t bins_with_signal_reverse;

        uint16_t maxBinIndex;
        uint16_t minBinIndex;

        unsigned long timestamp; // ms of how long the sensor has been running

        void process(float* data, uint16_t iq_offset, float noiseFloorDistanceThreshold, float speedConversion);
    };

  public:
    AudioSystem();

    void setup(Config const& config, float maxPedestrianSpeed, float sendMaxSpeed);
    void processData(Results& results);

    bool hasData();
    void updateIQ(Config const& config);

    uint16_t getNumberOfFftBins() const { return numberOfFftBins; }
    float getPeak() { return peak1.read(); }

  private:
    Config config;

    uint16_t iq_offset;       // offset used for IQ calculation
    uint16_t numberOfFftBins; // is calculated from send_max_speed
    uint16_t maxBinIndex;
    uint16_t minBinIndex;
    uint16_t max_pedestrian_bin;

    AudioAnalyzeFFT1024_IQ_F32 fft_IQ1024;
    AudioAnalyzePeak_F32 peak1;
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
