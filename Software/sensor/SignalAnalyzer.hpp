#ifndef SIGNALANALYZER_HPP
#define SIGNALANALYZER_HPP

#include "statistics.hpp"

#include <cstdint>
#include <optional>

class SignalAnalyzer
{
  public:
    struct Config
    {
        size_t signalSampleRate = 12000;
        size_t fftWidth = 1024;
        bool isIqMeasurement = true; // measure in both directions?

        size_t runningMeanHistoryN = 100;   // number N used for running mean
        size_t hannWindowN = 21;            // number N used for Hann window smoothing
        size_t carSignalLengthMinimum = 10; // how much signals are required to treat it as car
        size_t carSignalBufferLength = 200; // how much past signals are required to be stored for calculation

        float noiseFloorDistance_threshold = 8.f; // dB; distance of "proper signal" to noise floor
        float signalStrengthThreshold = 20;       // relative signal over which it is considered to be a signal
        float carSignalThreshold = 0.2;           // threshold to process the signal as car trigger

        float noiseLevelMinSpeed = 10.f;  // km/h; speed under which signals do not contribute to noise calculation
        float maxPedestrianSpeed = 10.0f; // km/h; speed under which signals are detected as pedestrians
        float maxCarSpeed = 50.f;         // km/h; speed under which signals are detected as cars
        float maxTotalSpeed = 500.f;      // km/h; don't send/store/write spectral data higher than this speed

        // Config& operator=(Config const& other);
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
        float speedConversionFactor = 0;

        IndexBand binsToProcess;  // limited by +- maxTotalSpeed
        IndexBand noiseLevelBins; // limited by noiseLevelMinSpeed and maxTotalSpeed
        IndexBand carBins;        // limited by maxPedestrianSpeed and min(maxCarSpeed,maxTotalSpeed)
        IndexBand pedestrianBins; // limited by min(maxPedestrianSpeed,maxTotalSpeed)
    };

    struct Results
    {
        struct DirectionalData
        {
            float maxAmplitude = -9999;    // highest signal in spectrum
            float maxSignalStrength = 0.0; // speed in m/s based on peak frequency
            float detectedSpeed = 0.0;     // speed in m/s based on peak frequency
            size_t maxAmplitudeBin = 0;
            bool considerAsSignal = false;
        };
        struct CommonData
        {
            float meanAmplitudeForPedestrians = 0.0; // used to detect the presence of a pedestrians
            float meanAmplitudeForCars = 0.0;        // mean amplitude in bins for at max car speed
            float meanAmplitudeForNoiseLevel = 0.0;  // mean amplitude over entire spectrum

            float dynamicNoiseLevel = 0.0; // simple running mean over X values
            float carTriggerSignal = 0.0;  // smoothed meanAmpForCars (using Hann-window)
        };

        struct ObjectDetection
        {
            bool isForward = true;     // forward or reverse
            size_t speedMarkerAge = 0; // how many samples back the speed recording should start; depending on the
                                       // direction we use different values (min/max) here
            size_t validSpeedCount = 0;

            unsigned long timestamp = 0; // ms of how long the sensor has been running
            float medianSpeed = 0.0f;
            std::vector<float> speeds;
        };

        void reset(SetupData const& setupData, size_t fftWidth);
        void process(float noiseFloorDistanceThreshold, float speedConversion);

      public:
        unsigned long timestamp; // ms of how long the sensor has been running
        SetupData setupData;

        std::vector<float> noiseFloorDistance;
        std::vector<float> spectrum; // spectral data

        DirectionalData forward; // result data for forward direction
        DirectionalData reverse; // result data for backward direction
        CommonData data;         // result data independent of any direction
        std::optional<ObjectDetection> completedForwardDetection;
        std::optional<ObjectDetection> completedReverseDetection;
    };

  public:
    void setup(Config const& config);
    void processData(Results& results);

    Config const& getConfig() const { return config; }

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
            size_t startAge = 0;     // start of the scan
            size_t maxSignalAge = 0; // where the maximum value was
            size_t minSignalAge = 0; // where the minimum value was
        };

        RunningMean dynamicNoiseLevel;
        HannWindowSmoothing carTriggerSignal;

        SignalScan signalScan;

        std::optional<Results::ObjectDetection> incompleteForwardDetection;

        bool hasPastDetection = false; // did we have any trigger events yet
        size_t lastSpeedMarkerAge = 0; // how many samples back the last trigger has been processed

        RingBuffer<float> forwardSpeeds;
        RingBuffer<float> reverseSpeeds;
    };

  private:
    void useAndUpdateHistory(Results& results, History& history);
    void finalize(Results::ObjectDetection& detection);

  private:
    Config config;
    SetupData setupData;
    History history;
};

#endif
