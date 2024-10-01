#include "AudioSystem.h"

#include <TimeLib.h>

#include "noise_floor.h"

AudioSystem::AudioSystem()
    : patchCord1(linein, 0, I_gain, 0)
    , patchCord2(I_gain, 0, fft_IQ1024, 0)
    , patchCord3(linein, 0, Q_mixer, 0)
    , patchCord3b(linein, 1, Q_mixer, 1)
    , patchCord4(Q_mixer, 0, fft_IQ1024, 1)
    , patchCord5(linein, 0, peak1, 0)
    , patchCord6(linein, 0, headphone, 0)
    , patchCord7(linein, 1, headphone, 1)
{}

void AudioSystem::setup(AudioSystem::Config const& config, float maxPedestrianSpeed, float maxSpeedToUse)
{
    this->config = config;

    // Audio connections require memory to work. For more detailed information, see the MemoryAndCpuUsage example
    AudioMemory_F32(400);

    sgtl5000_1.enable();
    sgtl5000_1.inputSelect(config.audioInput);  // AUDIO_INPUT_LINEIN or AUDIO_INPUT_MIC
    sgtl5000_1.micGain(config.micGain);         // only relevant if AUDIO_INPUT_MIC is used
    sgtl5000_1.lineInLevel(config.lineInLevel); // only relevant if AUDIO_INPUT_LINEIN is used
    sgtl5000_1.volume(.5);

    updateIQ(config);

    fft_IQ1024.windowFunction(AudioWindowHanning1024);
    fft_IQ1024.setNAverage(1);
    fft_IQ1024.setOutputType(FFT_DBFS); // FFT_RMS or FFT_POWER or FFT_DBFS
    fft_IQ1024.setXAxis(3);

    speedConversion = 1.0 * (config.sampleRate / AudioSystem::fftWidth) / 44.0; // conversion from Hz to km/h
    setupData.pedestriansBinMax = maxPedestrianSpeed / speedConversion;         // convert maxPedestrianSpeed to bin
    uint16_t rawBinCount = (uint16_t)min(AudioSystem::fftWidth / 2, maxSpeedToUse / speedConversion);

    // IQ = symetric FFT
    if(config.isIqMeasurement)
    {
        setupData.iqOffset = AudioSystem::fftWidth / 2; // new middle point
        setupData.numberOfFftBins = rawBinCount * 2;    // send both sides; not only one
        setupData.maxBinIndex = setupData.numberOfFftBins;
        setupData.minBinIndex = (AudioSystem::fftWidth - setupData.maxBinIndex);
    }
    else
    {
        setupData.iqOffset = 0;
        setupData.minBinIndex = 0;
        setupData.numberOfFftBins = rawBinCount;
    }

    history.runningMeanForward = RunningMean(config.runningMeanHistoryN, 0.0f);
    history.runningMeanReverse = RunningMean(config.runningMeanHistoryN, 0.0f);

    history.smoothedAmpForward = HannWindowSmoothing(config.hannWindowN);
    history.smoothedAmpReverse = HannWindowSmoothing(config.hannWindowN);
}

void AudioSystem::processData(Results& results)
{
    // prepare
    results.timestamp = millis();
    results.setupData = this->setupData;
    results.pedestrianAmplitude = 0.0;

    // do stuff
    results.process(fft_IQ1024.getData(), config.noiseFloorDistance_threshold, speedConversion);

    // use and update history
    useAndUpdateHistory(results, this->history);
}

void AudioSystem::Results::process(float* pointer, float noiseFloorDistanceThreshold, float speedConversion)
{
    for(size_t i = 0; i < AudioSystem::fftWidth; i++)
    {
        spectrum[i] = pointer[i];
        noiseFloorDistance[i] = spectrum[i] - global_noiseFloor[i];
    }

    forward = reverse = Data{};

    // detect pedestrians
    pedestrianAmplitude = 0.0;
    for(size_t i = 3 + setupData.iqOffset; i < setupData.pedestriansBinMax + setupData.iqOffset; i++)
        pedestrianAmplitude += spectrum[i];
    pedestrianAmplitude /= setupData.pedestriansBinMax;

    // detect faster things
    size_t const usedBinsStartIdx = (setupData.pedestriansBinMax + 1 + setupData.iqOffset);
    for(size_t i = usedBinsStartIdx; i < setupData.maxBinIndex; i++)
    {
        float& forwardValue = noiseFloorDistance[i];
        float& reverseValue = noiseFloorDistance[AudioSystem::fftWidth - i];

        forward.meanAmplitude += forwardValue;
        if(forwardValue > noiseFloorDistanceThreshold)
            forward.binsWithSignal++;

        reverse.meanAmplitude += reverseValue;
        if(reverseValue > noiseFloorDistanceThreshold)
            reverse.binsWithSignal++;

        // with noiseFloorDistance[i] > noiseFloorDistance[1024-i] make sure that the signal is in the right
        // direction
        if(forwardValue > reverseValue && forwardValue > forward.amplitudeMax)
        {
            forward.amplitudeMax = forwardValue; // remember highest amplitude
            forward.maxFrequencyIdx = i;         // remember frequency index
        }
        if(reverseValue > forwardValue && reverseValue > forward.amplitudeMax)
        {
            reverse.amplitudeMax = reverseValue; // remember highest amplitude
            reverse.maxFrequencyIdx = i;         // remember frequency index
        }
    }
    forward.detectedSpeed = (forward.maxFrequencyIdx - setupData.iqOffset) * speedConversion;
    reverse.detectedSpeed = (reverse.maxFrequencyIdx - setupData.iqOffset) * speedConversion;

    // TODO: is this valid when working with dB values?
    auto const binCount = setupData.maxBinIndex - usedBinsStartIdx;
    forward.meanAmplitude /= binCount;
    reverse.meanAmplitude /= binCount;
}

bool AudioSystem::hasData()
{
    return fft_IQ1024.available();
}

void AudioSystem::updateIQ(Config const& config)
{
    // after https://www.faculty.ece.vt.edu/swe/argus/iqbal.pdf
    float A = 1 / config.alpha;
    float C = -sin(config.psi) / (config.alpha * cos(config.psi));
    float D = 1 / cos(config.psi);

    I_gain.setGain(A);
    Q_mixer.gain(0, C);
    Q_mixer.gain(1, D);
}

void AudioSystem::useAndUpdateHistory(Results& results, AudioSystem::History& history)
{
    results.forward.runningMeanAmp = history.runningMeanForward.add(results.forward.meanAmplitude);
    results.reverse.runningMeanAmp = history.runningMeanReverse.add(results.reverse.meanAmplitude);

    // TODO: this could be limited to a mean amp covering 0-50km/h only
    results.forward.carTriggerSignal = history.smoothedAmpForward.add(results.forward.meanAmplitude);
    results.reverse.carTriggerSignal = history.smoothedAmpReverse.add(results.reverse.meanAmplitude);
}

AudioSystem::Config& AudioSystem::Config::operator=(const Config& other)
{
    micGain = other.micGain;
    alpha = other.alpha;
    psi = other.psi;

    return *this;
}
