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

void AudioSystem::setup(AudioSystem::Config const& config)
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
    uint16_t rawBinCount = (uint16_t)min(AudioSystem::fftWidth / 2, config.maxTotalSpeed / speedConversion);

    // note for IQ: bins[511] is the mean value and not used here; because of that we would have one more value in the
    // forward direction which we are going to ignore instead. This then leads to having a symetric range.
    setupData.binsToProcess.from = 0;
    setupData.binsToProcess.to = config.isIqMeasurement ? rawBinCount - 1 : rawBinCount;

    // Note: IQ = symetric FFT; middle point or offset = 511 for FFT(1024)
    setupData.iqOffset = config.isIqMeasurement ? AudioSystem::fftWidth / 2 - 1 : 0;

    uint16_t pedBinCount = config.maxPedestrianSpeed / speedConversion;
    uint16_t noiseLevelBinMin = config.noiseLevelMinSpeed / speedConversion;
    uint16_t carBinMax = config.maxCarSpeed / speedConversion;

    setupData.pedestrianBins.from = 3; // magic number
    setupData.pedestrianBins.to = std::min(pedBinCount, rawBinCount);

    setupData.noiseLevelBins.from = std::min(noiseLevelBinMin, rawBinCount);
    setupData.noiseLevelBins.to = rawBinCount;

    setupData.carBins.from = std::min(pedBinCount, rawBinCount);
    setupData.carBins.to = std::min(carBinMax, rawBinCount);

    history.dynamicNoiseLevel = RunningMean(config.runningMeanHistoryN, 0.0f);
    history.carTriggerSignal = HannWindowSmoothing(config.hannWindowN);
    history.speeds = RingBuffer<float>{config.carSignalBufferLength, std::numeric_limits<float>::quiet_NaN()};

    // Serial.println(setupData.carBinMax);   // as of now 200
    // Serial.println(setupData.minBinIndex); // 0
    // Serial.println(setupData.maxBinIndex); // 1023
}

void AudioSystem::processData(Results& results)
{
    // prepare
    results.reset(millis(), this->setupData);

    // do stuff
    results.process(fft_IQ1024.getData(), config.noiseFloorDistance_threshold, speedConversion);

    // use and update history
    useAndUpdateHistory(results, this->history);
}

void AudioSystem::Results::reset(unsigned long timestamp, SetupData const& setupData)
{
    this->timestamp = timestamp;
    this->setupData = setupData;

    forward = reverse = DirectionalData{};
    data = CommonData{};
    completedForwardDetection.reset();
    completedReverseDetection.reset();

    // do not touch noiseFloorDistance and spectrum, since they are expensive and overwritten for sure
}

void AudioSystem::Results::process(float* pointer, float noiseFloorDistanceThreshold, float speedConversion)
{
    for(size_t i = 0; i < AudioSystem::fftWidth; i++)
    {
        spectrum[i] = pointer[i];
        noiseFloorDistance[i] = spectrum[i] - global_noiseFloor[i];
    }

    // detect pedestrians - this is somewhat different to the other calculations
    float sum = 0.0;
    if(setupData.pedestrianBins.count() > 0)
    {
        for(size_t i = setupData.pedestrianBins.from; i <= setupData.pedestrianBins.to; i++)
            sum += spectrum[setupData.iqOffset + i];
        data.meanAmplitudeForPedestrians = sum / setupData.pedestrianBins.count();
    }

    uint16_t maxAmplitudeBinForward = 0;
    uint16_t maxAmplitudeBinReverse = 0;
    bool const doReverse = setupData.iqOffset > 0; // same as config.isIqMeasurement
    for(size_t i = setupData.binsToProcess.from; i <= setupData.binsToProcess.to; i++)
    {
        float const forwardValue = noiseFloorDistance[setupData.iqOffset + 1 + i]; // from 512 to 1022 = 510 values
        float const reverseValue = doReverse ? noiseFloorDistance[setupData.iqOffset - i - 1] : 0;

        if(setupData.noiseLevelBins.contains(i))
            data.meanAmplitudeForNoiseLevel += forwardValue + reverseValue;

        if(setupData.carBins.contains(i))
        {
            data.meanAmplitudeForCars += forwardValue + reverseValue;

            if(forwardValue > noiseFloorDistanceThreshold)
                forward.binsWithSignal++;
            if(reverseValue > noiseFloorDistanceThreshold)
                reverse.binsWithSignal++;

            // the value has to be greater than the ghost signal on the other side to be considered
            if(forwardValue > reverseValue && forwardValue > forward.maxAmplitude)
            {
                forward.maxAmplitude = forwardValue; // remember highest amplitude
                maxAmplitudeBinForward = i;          // remember frequency index
            }
            if(reverseValue > forwardValue && reverseValue > reverse.maxAmplitude)
            {
                reverse.maxAmplitude = reverseValue; // remember highest amplitude
                maxAmplitudeBinReverse = i;          // remember frequency index
            }
        }
    }
    forward.detectedSpeed = maxAmplitudeBinForward * speedConversion;
    reverse.detectedSpeed = maxAmplitudeBinReverse * speedConversion;

    // TODO: is this valid when working with dB values?
    data.meanAmplitudeForNoiseLevel /= (setupData.noiseLevelBins.count() * (doReverse ? 2 : 1));
    data.meanAmplitudeForCars /= (setupData.carBins.count() * (doReverse ? 2 : 1));
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
    results.data.dynamicNoiseLevel = history.dynamicNoiseLevel.add(results.data.meanAmplitudeForNoiseLevel);

    float lastCarSignal = history.carTriggerSignal.get();
    results.data.carTriggerSignal = history.carTriggerSignal.add(results.data.meanAmplitudeForCars);

    // Serial.print("last: ");
    // Serial.println(lastCarSignal);
    // Serial.print("mean: ");
    // Serial.println(results.data.meanAmplitudeForCars);

    auto scan =
        [this, &history, lastCarSignal](Results::CommonData const& data) -> std::optional<Results::ObjectDetection> {
        // if there is no value in the buffer, we just got our first measurement in and can end here
        if(std::isnan(lastCarSignal))
            return {};

        float const carTriggerSignalDiff = data.carTriggerSignal - lastCarSignal;

        auto& signalScan = history.signalScan;
        // FLOW: if we are not in any collection phase, we are waiting for a strong signal to occur
        if(signalScan.isCollecting == false)
        {
            if(carTriggerSignalDiff < this->config.carSignalThreshold)
                return {};

            // Serial.print("delta: ");
            // Serial.println(carTriggerSignalDiff);

            // FLOW: we got a big enough difference over the threshold to start collecting data
            signalScan = History::SignalScan{};
            signalScan.isCollecting = true; // yes, go ahead
            signalScan.collectMax = true;   // select the maximum first
            signalScan.collectMin = false;  // but not the minimum quiet yet
            signalScan.startOffset = 0;
        }
        else
        {
            // age all offsets; even if they are not set yet
            signalScan.startOffset++;
            signalScan.endOffset++;
            signalScan.maxOffset++;
            signalScan.minOffset++;
        }

        // FLOW: we are looking for the maximum now
        if(signalScan.collectMax)
        {
            if(carTriggerSignalDiff >= 0)
            {
                if(carTriggerSignalDiff > signalScan.maxSignal)
                {
                    signalScan.maxSignal = carTriggerSignalDiff;
                    signalScan.maxOffset = 0;
                }
            }
            else
            {
                // FLOW: change to looking for the minimum now
                signalScan.collectMax = false;
                signalScan.collectMin = true;
            }
        }

        // FLOW: we are looking for the minimum now
        if(signalScan.collectMin)
        {
            if(carTriggerSignalDiff < 0)
            {
                if(carTriggerSignalDiff < signalScan.minSignal)
                {
                    signalScan.minSignal = carTriggerSignalDiff;
                    signalScan.minOffset = 0;
                }
            }
            else
            {
                // FLOW: we are back to positive values. Stop collecting.
                signalScan.isCollecting = false;
                signalScan.endOffset = 0;
            }
        }

        // FLOW: we are not done collecting yet
        if(signalScan.isCollecting)
            return {};

        // FLOW: we can continue with the result analysis
        bool isLongEnough = (signalScan.maxOffset - signalScan.minOffset + 1) >= config.carSignalLengthMinimum;
        if(not isLongEnough)
            return {};

        bool isForwardSignal = std::abs(signalScan.maxSignal) > std::abs(signalScan.minSignal);

        Results::ObjectDetection detection;
        detection.isForward = isForwardSignal;
        detection.sampleCount = signalScan.startOffset - signalScan.endOffset + 1;

        return detection;
    };

    if(history.hasPastDetection)
        history.lastDetectionAge++;

    {
        float const relativeSignal = results.data.meanAmplitudeForCars - results.data.dynamicNoiseLevel;
        bool const considerAsSignal = relativeSignal > config.signalStrengthThreshold;

        history.speeds.add(considerAsSignal ? relativeSignal : std::numeric_limits<float>::quiet_NaN());

        // Serial.print(considerAsSignal);
        // Serial.print(" ");
        Serial.print(history.speeds.get(0));
        Serial.println(" ");
    }

    std::optional<Results::ObjectDetection> newDetection = scan(results.data);

    // let's see if we have any unfinished business
    bool needToFinalizePendingDetection =
        history.incompleteForwardDetection.has_value() &&
        (newDetection.has_value() || history.lastDetectionAge >= config.carSignalBufferLength);
    if(needToFinalizePendingDetection)
    {
        size_t valuesToUse = std::min(config.carSignalBufferLength, history.lastDetectionAge);
        history.incompleteForwardDetection.value().speeds = history.speeds.getLastN(valuesToUse);
        finalize(history.incompleteForwardDetection.value());
        results.completedForwardDetection = std::move(history.incompleteForwardDetection);
        history.incompleteForwardDetection.reset();
    }

    if(newDetection.has_value())
    {
        auto& detection = newDetection.value();
        detection.timestamp = results.timestamp;
        detection.speeds.reserve(config.carSignalBufferLength);

        if(detection.isForward)
        {
            history.incompleteForwardDetection = std::move(newDetection);
        }
        else
        {
            size_t valuesToUse = std::min(config.carSignalBufferLength, history.lastDetectionAge);
            detection.speeds = history.speeds.getLastN(valuesToUse);
            finalize(detection);
            results.completedReverseDetection = std::move(newDetection);
        }

        history.hasPastDetection = true;
        history.lastDetectionAge = 0;
    }
}

void AudioSystem::finalize(Results::ObjectDetection& detection)
{
    // the speeds vector will contain naNs which we need to filter out
    auto speeds = filterValid(detection.speeds);
    Serial.println("speeds");
    for(auto const& s : speeds)
        Serial.print(s);
    Serial.println("");
    detection.medianSpeed = getAlmostMedian(speeds);
}

AudioSystem::Config& AudioSystem::Config::operator=(const Config& other)
{
    micGain = other.micGain;
    alpha = other.alpha;
    psi = other.psi;

    return *this;
}
