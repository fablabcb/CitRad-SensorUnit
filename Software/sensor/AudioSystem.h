#ifndef AUDIOSYSTEM_H
#define AUDIOSYSTEM_H

#include "statistics.hpp"

#include <Audio.h>
#include <AudioStream_F32.h>
#include <OpenAudio_ArduinoLibrary.h>

#include <array>
#include <cstddef>
#include <optional>

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
        size_t runningMeanHistoryN = 100;             // number N used for running mean
        size_t hannWindowN = 31;                      // number N used for Hann window smoothing
        size_t carSignalLengthMinimum = 10;           // how much signals are required to treat it as car
        size_t carSignalBufferLength = 200;           // how much past signals are required to be stored for calculation
        float signalStrengthThreshold = 20;           // relative signal over which it is considered to be a signal
        float carSignalThreshold = 5 / 30;            // threshold to process the signal as car trigger

        float noiseLevelMinSpeed = 10.0; // km/h; speed under which signals do not contribute to noise calculation
        float maxPedestrianSpeed = 10.0; // km/h; speed under which signals are detected as pedestrians
        float maxCarSpeed = 50.0;        // km/h; speed under which signals are detected as cars
        float maxTotalSpeed = 500;       // km/h; don't send/store/write spectral data higher than this speed

        // IQ calibration
        bool hasChanges = false;
        float alpha = 1.10;
        float psi = -0.04;

        Config& operator=(Config const& other);
    };

    /**
     * @brief The IndexBand struct defines a relative band of FFT-bin indices to be used for a certain operation.
     * @note When using the IQ measurement, the iqOffset needs to be taken into account to get the real index into the
     * FFT data.
     */
    struct IndexBand
    {
        uint16_t from = 0;
        uint16_t to = 0;

        uint16_t count() const { return from > to ? 0 : (to - from + 1); }
        bool contains(size_t index) const { return from <= index && index <= to; }
    };

    // data derived from config
    struct SetupData
    {
        uint16_t iqOffset; // offset used for index ranges when doing IQ calculation

        IndexBand binsToProcess;  // limited by +- maxTotalSpeed
        IndexBand noiseLevelBins; // limited by noiseLevelMinSpeed and maxTotalSpeed
        IndexBand carBins;        // limited by maxPedestrianSpeed and min(maxCarSpeed,maxTotalSpeed)
        IndexBand pedestrianBins; // limited by min(maxPedestrianSpeed,maxTotalSpeed)
    };

    struct Results
    {
        struct DirectionalData
        {
            float maxAmplitude = -9999; // highest signal in spectrum
            float detectedSpeed = 0.0;  // speed in m/s based on peak frequency

            uint8_t binsWithSignal = 0; // how many bins have signal over the noise threshold? // TODO unused?
        };
        struct CommonData
        {
            float meanAmplitudeForPedestrians;      // used to detect the presence of a pedestrians
            float meanAmplitudeForCars = 0.0;       // mean amplitude in bins for at max car speed
            float meanAmplitudeForNoiseLevel = 0.0; // mean amplitude over entire spectrum

            float dynamicNoiseLevel = 0.0; // simple running mean over X values
            float carTriggerSignal = 0.0;  // smoothed meanAmpForCars (using Hann-window)
        };

        struct ObjectDetection
        {
            bool isForward = true;  // forward or reverse
            size_t sampleCount = 0; // how many samples this detection contains

            unsigned long timestamp = 0; // ms of how long the sensor has been running
            float medianSpeed = 0.0f;

            std::vector<float> speeds;
        };

        void reset(unsigned long timestamp, SetupData const& setupData);
        void process(float* data, float noiseFloorDistanceThreshold, float speedConversion);

      public:
        unsigned long timestamp; // ms of how long the sensor has been running
        SetupData setupData;

        std::array<float, fftWidth> noiseFloorDistance;
        std::array<float, fftWidth> spectrum; // spectral data

        DirectionalData forward; // result data for forward direction
        DirectionalData reverse; // result data for backward direction
        CommonData data;         // result data independent of any direction
        std::optional<ObjectDetection> completedForwardDetection;
        std::optional<ObjectDetection> completedReverseDetection;
    };

  public:
    AudioSystem();

    void setup(Config const& config);
    void processData(Results& results);

    bool hasData();
    void updateIQ(Config const& config);

    float getPeak() const { return peak1.read(); }

  private:
    struct History
    {
        struct SignalScan
        {
            bool isCollecting = false;
            bool collectMax = false;
            bool collectMin = false;

            float maxSignal = 0; // absolute value of the max signal processed
            float minSignal = 0; // absolute value of the min signal processed

            // These offsets point into the past and define how many samples back a certain event was.
            size_t startOffset = 0; // start of the scan
            size_t endOffset = 0;   // end of the scan
            size_t maxOffset = 0;   // where the maximum value was
            size_t minOffset = 0;   // where the minimum value was
        };

        RunningMean dynamicNoiseLevel;
        HannWindowSmoothing carTriggerSignal;

        SignalScan signalScan;

        std::optional<Results::ObjectDetection> incompleteForwardDetection;

        bool hasPastDetection = false; // did we have any trigger events yet
        size_t lastDetectionAge = 0;   // how many samples back the last trigger has been processed // TODO

        RingBuffer<float> speeds;
    };

    void useAndUpdateHistory(Results& results, History& history);
    void finalize(Results::ObjectDetection& detection);

  private:
    Config config;
    SetupData setupData;

    History history;

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
