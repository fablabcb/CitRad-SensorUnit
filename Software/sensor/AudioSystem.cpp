#include "AudioSystem.h"

#include <TimeLib.h>
#include <cmath>

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

    history.forward.dynamicNoiseLevel = RunningMean(config.runningMeanHistoryN, 0.0f);
    history.reverse.dynamicNoiseLevel = RunningMean(config.runningMeanHistoryN, 0.0f);

    history.forward.carTriggerSignal = HannWindowSmoothing(config.hannWindowN);
    history.reverse.carTriggerSignal = HannWindowSmoothing(config.hannWindowN);

    history.speeds = RingBuffer<float>{config.carSignalBufferLength, ~0};
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

    // TODO adjust this to:
    // use entire range for noise floor
    // use -50km/h to +50km/h for meanAmp

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

        forward.signalStrength += forwardValue;
        if(forwardValue > noiseFloorDistanceThreshold)
            forward.binsWithSignal++;

        reverse.signalStrength += reverseValue;
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
    forward.signalStrength /= binCount;
    reverse.signalStrength /= binCount;
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
    std::optional<History::Trigger> resultTrigger;

    auto update = [this, &resultTrigger](Results::Data& resultData, AudioSystem::History::Data& historyData) {
        resultData.dynamicNoiseLevel = historyData.dynamicNoiseLevel.add(resultData.signalStrength);

        // TODO: this could be limited to a mean amp covering 0-50km/h only
        float tmp = historyData.carTriggerSignal.get();
        resultData.carTriggerSignal = historyData.carTriggerSignal.add(resultData.signalStrength);

        // if there is no value in the buffer, we just got our first measurement in and can end here
        if(std::isnan(tmp))
            return;

        float const carTriggerSignalDiff = resultData.signalStrength - tmp;

        auto& signalScan = historyData.signalScan;
        // if we are not in any collection phase, we are waiting for a strong signal to occur
        if(signalScan.isCollecting == false)
        {
            if(carTriggerSignalDiff < config.carSignalThreshold)
                return;

            signalScan = History::SignalScan{};
            signalScan.isCollecting = true; // yes, go ahead
            signalScan.collectMax = true;   // select the maximum first
            signalScan.collectMin = false;  // and the minimum not quiet yet
            signalScan.startOffset = 0;
        }

        if(signalScan.collectMax)
        {
            if(carTriggerSignalDiff >= 0)
            {
                signalScan.startOffset++;

                if(carTriggerSignalDiff > signalScan.maxSignal)
                {
                    signalScan.maxSignal = carTriggerSignalDiff;
                    signalScan.maxOffset = 0;
                }
            }
            else
            {
                signalScan.collectMax = false;
                signalScan.collectMin = true;
                signalScan.endOffset = 0;
            }
        }

        if(signalScan.collectMin)
        {
            if(carTriggerSignalDiff <= 0)
            {
                signalScan.endOffset++;

                if(carTriggerSignalDiff < signalScan.minSignal)
                {
                    signalScan.minSignal = carTriggerSignalDiff;
                    signalScan.minOffset = 0;
                }
            }
            else
            {
                signalScan.isCollecting = false;
            }
        }

        // we are not done collecting yet
        if(signalScan.isCollecting)
            return;

        // note: Due to the way they are started, both signalScan.startOffset and signalScan.endOffset are one too heigh

        bool isLongEnough = (signalScan.minOffset - signalScan.maxOffset) > config.carSignalLengthMinimum;
        if(not isLongEnough)
            return;

        bool isForwardSignal = std::abs(signalScan.maxSignal) > std::abs(signalScan.minSignal);

        History::Trigger trigger;
        trigger.isForward = isForwardSignal;
        trigger.sampleCount = signalScan.startOffset - signalScan.endOffset;

        resultTrigger = trigger;
    };

    if(history.hasPastTrigger)
        history.lastTriggerAge++;

    {
        float const relativeSignal = results.forward.signalStrength - results.forward.dynamicNoiseLevel;
        bool const considerAsSignal = relativeSignal > config.signalStrengthThreshold;

        history.speeds.set(considerAsSignal ? relativeSignal : ~0); // ~0 is naN
    }

    update(results.reverse, history.reverse);
    // we will only have one trigger in the end .. wait until the process function has been altered
    resultTrigger.reset();

    update(results.forward, history.forward);

    // let's see if we have any unfinished business
    bool needToFinalizePendingTrigger =
        history.activeIncompleteTrigger.has_value() &&
        (resultTrigger.has_value() || history.lastTriggerAge >= config.carSignalBufferLength);
    if(needToFinalizePendingTrigger)
    {
        size_t valuesToUse = std::min(config.carSignalBufferLength, history.lastTriggerAge);
        history.activeIncompleteTrigger.value().speeds = history.speeds.getLastN(valuesToUse);
        finalizeAndStore(history.activeIncompleteTrigger.value());
        history.activeIncompleteTrigger.reset();
    }

    if(resultTrigger.has_value())
    {
        auto& trigger = resultTrigger.value();
        trigger.timestamp = results.timestamp;
        trigger.speeds.reserve(config.carSignalBufferLength);

        if(trigger.isForward)
        {
            history.activeIncompleteTrigger = std::move(resultTrigger);
        }
        else
        {
            size_t valuesToUse = std::min(config.carSignalBufferLength, history.lastTriggerAge);
            trigger.speeds = history.speeds.getLastN(valuesToUse);
            finalizeAndStore(trigger);
        }

        history.hasPastTrigger = true;
        history.lastTriggerAge = 0;
    }
}

void AudioSystem::finalizeAndStore(History::Trigger& trigger)
{
    // the speeds vector will contain naNs which we need to filter out
    auto speeds = filterValid(trigger.speeds);
    trigger.medianSpeed = getAlmostMedian(speeds);

    // TODO save
}

AudioSystem::Config& AudioSystem::Config::operator=(const Config& other)
{
    micGain = other.micGain;
    alpha = other.alpha;
    psi = other.psi;

    return *this;
}
