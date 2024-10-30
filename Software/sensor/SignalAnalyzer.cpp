#include "SignalAnalyzer.hpp"

#include "noise_floor.h"

#include <algorithm>
#include <cmath>
#include <limits>

void SignalAnalyzer::setup(SignalAnalyzer::Config const& config)
{
    this->config = config;

    float speedConversion = 1.0 * config.signalSampleRate / (44.0 * config.fftWidth); // conversion from Hz to km/h
    setupData.speedConversionFactor = speedConversion;
    uint16_t rawBinCount = std::min(config.fftWidth / 2, size_t(config.maxTotalSpeed / speedConversion));

    // note for IQ: bins[511] is the mean value and not used here; because of that we would have one more value in the
    // forward direction which we are going to ignore instead. This then leads to having a symetric range.
    setupData.binsToProcess.from = 1;
    setupData.binsToProcess.to = config.isIqMeasurement ? rawBinCount - 1 : rawBinCount;

    // Note: IQ = symetric FFT; middle point or offset = 511 for FFT(1024)
    setupData.iqOffset = config.isIqMeasurement ? config.fftWidth / 2 - 1 : 0;

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
    history.forwardSpeeds = RingBuffer<float>{config.carSignalBufferLength, std::numeric_limits<float>::quiet_NaN()};
    history.reverseSpeeds = RingBuffer<float>{config.carSignalBufferLength, std::numeric_limits<float>::quiet_NaN()};
}

void SignalAnalyzer::processData(Results& results)
{
    results.reset(this->setupData, config.fftWidth);

    results.process(config.noiseFloorDistance_threshold, setupData.speedConversionFactor);

    useAndUpdateHistory(results, this->history);
}

void SignalAnalyzer::Results::reset(SetupData const& setupData, size_t fftWidth)
{
    this->setupData = setupData;

    noiseFloorDistance.resize(fftWidth);
    spectrum.resize(fftWidth);

    forward = reverse = DirectionalData{};
    data = CommonData{};
    completedForwardDetection.reset();
    completedReverseDetection.reset();

    // do not touch noiseFloorDistance and spectrum, since they are expensive and overwritten for sure
}

void SignalAnalyzer::Results::process(float noiseFloorDistanceThreshold, float speedConversion)
{
    for(size_t i = 0; i < spectrum.size(); i++)
        noiseFloorDistance[i] = spectrum[i] - global_noiseFloor[i];

    // detect pedestrians - this is somewhat different to the other calculations
    float sum = 0.0;
    if(setupData.pedestrianBins.count() > 0)
    {
        for(size_t i = setupData.pedestrianBins.from; i <= setupData.pedestrianBins.to; i++)
            sum += spectrum[setupData.iqOffset + i];
        data.meanAmplitudeForPedestrians = sum / setupData.pedestrianBins.count();
    }

    bool const doReverse = setupData.iqOffset > 0; // same as config.isIqMeasurement
    for(size_t i = setupData.binsToProcess.from; i <= setupData.binsToProcess.to; i++)
    {
        float const forwardValue = noiseFloorDistance[setupData.iqOffset + i]; // from 512 to 1022 = 511 values
        float const reverseValue = doReverse ? noiseFloorDistance[setupData.iqOffset - i] : 0;

        if(setupData.noiseLevelBins.contains(i))
            data.meanAmplitudeForNoiseLevel += forwardValue + reverseValue;

        if(setupData.carBins.contains(i))
        {
            float const sForwardValue = spectrum[setupData.iqOffset + i];
            float const sReverseValue = doReverse ? spectrum[setupData.iqOffset - i] : 0;
            data.meanAmplitudeForCars += sForwardValue + sReverseValue;
        }

        if(forwardValue > noiseFloorDistanceThreshold)
            forward.binsWithSignal++;
        if(reverseValue > noiseFloorDistanceThreshold)
            reverse.binsWithSignal++;

        // the value has to be greater than the ghost signal on the other side to be considered
        if(forwardValue > reverseValue && forwardValue > forward.maxAmplitude)
        {
            forward.maxAmplitude = forwardValue; // remember highest amplitude
            forward.maxAmplitudeBin = i;         // remember frequency index
        }
        if(reverseValue > forwardValue && reverseValue > reverse.maxAmplitude)
        {
            reverse.maxAmplitude = reverseValue; // remember highest amplitude
            reverse.maxAmplitudeBin = i;         // remember frequency index
        }
    }

    forward.maxSignalStrength = forward.maxAmplitude;
    reverse.maxSignalStrength = reverse.maxAmplitude;
    forward.detectedSpeed = 1.0 * forward.maxAmplitudeBin * speedConversion;
    reverse.detectedSpeed = 1.0 * reverse.maxAmplitudeBin * speedConversion;

    // TODO: is this valid when working with dB values?
    data.meanAmplitudeForNoiseLevel /= (setupData.noiseLevelBins.count() * (doReverse ? 2 : 1));
    data.meanAmplitudeForCars /= (setupData.carBins.count() * (doReverse ? 2 : 1));
}

void SignalAnalyzer::useAndUpdateHistory(Results& results, SignalAnalyzer::History& history)
{
    results.data.dynamicNoiseLevel = history.dynamicNoiseLevel.add(results.data.meanAmplitudeForNoiseLevel);

    float lastCarSignal = history.carTriggerSignal.get();
    results.data.carTriggerSignal = history.carTriggerSignal.add(results.data.meanAmplitudeForCars);

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

            // FLOW: we got a big enough difference over the threshold to start collecting data
            signalScan = History::SignalScan{};
            signalScan.isCollecting = true; // yes, go ahead
            signalScan.collectMax = true;   // select the maximum first
            signalScan.collectMin = false;  // but not the minimum quiet yet
            signalScan.startAge = 0;
        }
        else
        {
            // age all offsets; even if they are not set yet
            signalScan.startAge++;
            signalScan.maxSignalAge++;
            signalScan.minSignalAge++;
        }

        // FLOW: we are looking for the maximum now
        if(signalScan.collectMax)
        {
            if(carTriggerSignalDiff >= 0)
            {
                if(carTriggerSignalDiff > signalScan.maxSignal)
                {
                    signalScan.maxSignal = carTriggerSignalDiff;
                    signalScan.maxSignalAge = 0;
                }
            }
            else
            {
                //  FLOW: change to looking for the minimum now
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
                    signalScan.minSignalAge = 0;
                }
            }
            else
            {
                // FLOW: we are back to positive values. Stop collecting.
                signalScan.isCollecting = false;
            }
        }

        // FLOW: we are not done collecting yet
        if(signalScan.isCollecting)
            return {};

        // FLOW: we can continue with the result analysis if the signal is long enough
        if(bool isLongEnough = (signalScan.maxSignalAge - signalScan.minSignalAge + 1) >= config.carSignalLengthMinimum;
           not isLongEnough)
            return {};

        Results::ObjectDetection detection;
        detection.isForward = std::abs(signalScan.maxSignal) > std::abs(signalScan.minSignal);
        detection.speedMarkerAge = detection.isForward ? signalScan.minSignalAge : signalScan.maxSignalAge;

        return detection;
    };

    if(history.hasPastDetection)
        history.lastSpeedMarkerAge++;

    // record speed signal
    {
        float const relativeSignal = results.forward.maxSignalStrength - results.data.dynamicNoiseLevel;
        bool const considerAsSignal = relativeSignal > config.signalStrengthThreshold;
        history.forwardSpeeds.add(
            considerAsSignal ? results.forward.detectedSpeed : std::numeric_limits<float>::quiet_NaN());
        results.forward.considerAsSignal = considerAsSignal;
    }
    {
        float const relativeSignal = results.reverse.maxSignalStrength - results.data.dynamicNoiseLevel;
        bool const considerAsSignal = relativeSignal > config.signalStrengthThreshold;
        history.reverseSpeeds.add(
            considerAsSignal ? results.reverse.detectedSpeed : std::numeric_limits<float>::quiet_NaN());
        results.reverse.considerAsSignal = considerAsSignal;
    }

    // start/continue/end scanning for car signals
    std::optional<Results::ObjectDetection> newDetection = scan(results.data);

    // let's see if we have any unfinished business
    bool lastIsOldEnough = history.lastSpeedMarkerAge >= config.carSignalBufferLength;
    bool needToFinalizePendingDetection =
        history.incompleteForwardDetection.has_value() && (newDetection.has_value() || lastIsOldEnough);
    if(needToFinalizePendingDetection)
    {
        Range<size_t> speedValuesToUse{
            std::min(history.lastSpeedMarkerAge, config.hannWindowN / 2 + config.carSignalBufferLength),
            config.hannWindowN / 2 + (newDetection.has_value() ? newDetection->speedMarkerAge : 0)};

        history.incompleteForwardDetection->speeds =
            history.forwardSpeeds.getLastNButSkipFirst(speedValuesToUse.from, speedValuesToUse.to);
        finalize(history.incompleteForwardDetection.value());
        results.completedForwardDetection = history.incompleteForwardDetection;

        history.incompleteForwardDetection.reset();
        history.signalScan.isCollecting = false; // stop pending collection phase if any

        // regarding speeds: it should not be a problem if the tail of a max-length forward detection overlaps with the
        // next reverse detection; maybe even better to not cutoff the new detection
        // if(lastIsOldEnough) history.lastSpeedMarkerAge = 0;
    }

    if(newDetection.has_value())
    {
        auto& detection = newDetection.value();
        detection.timestamp = results.timestamp;
        detection.speeds.reserve(config.carSignalBufferLength);

        if(detection.isForward)
        {
            history.incompleteForwardDetection = newDetection;
        }
        else
        {
            Range<size_t> speedValuesToUse{
                std::min(config.carSignalBufferLength, config.hannWindowN / 2 + history.lastSpeedMarkerAge),
                config.hannWindowN / 2 + detection.speedMarkerAge};

            detection.speeds = history.reverseSpeeds.getLastNButSkipFirst(speedValuesToUse.from, speedValuesToUse.to);
            finalize(detection);
            results.completedReverseDetection = newDetection;
        }

        history.hasPastDetection = true;
        // history.lastSpeedMarkerAge = detection.speedMarkerAge;
        history.lastSpeedMarkerAge = history.signalScan.minSignalAge;
    }
}

void SignalAnalyzer::finalize(Results::ObjectDetection& detection)
{
    // the speeds vector will contain naNs which we need to filter out
    auto speeds = filterValid(detection.speeds);
    detection.medianSpeed = getAlmostMedian(speeds);
    detection.validSpeedCount = speeds.size();
}
