#include "SpectrumController.hpp"

SpectrumController::SpectrumController(size_t sampleSize, size_t maxSampleCount, size_t sampleRate)
    : samples(maxSampleCount)
    , sampleSize{sampleSize}
    , maxSampleCount{maxSampleCount}
    , sampleRate(sampleRate)
{
    speedToBinFactor = (44.0 * 1024.0) / sampleRate;
}

void SpectrumController::setMaxSampleCount(size_t newMaxSampleCount)
{
    if(maxSampleCount == newMaxSampleCount)
        return;

    maxSampleCount = newMaxSampleCount;

    samples = RingBuffer<std::vector<float>>(maxSampleCount);
    storedSamplesCount = 0;

    emit maxSampleCountChanged();
    emit storedSamplesCountChanged();
}

void SpectrumController::addSample(std::vector<float> const& sample, size_t timestamp)
{
    std::vector<float> s(sample);
    this->addSample(std::move(s), timestamp);
}

void SpectrumController::addSample(std::vector<float>&& sample, size_t timestamp)
{
    lastTimestamp = timestamp;
    samples.add(std::move(sample));
    activeSampleIdx++;

    if(storedSamplesCount < samples.size())
    {
        storedSamplesCount++;
        emit storedSamplesCountChanged();
    }
    emit activeSampleIdxChanged();
    emit sampleAdded();
}

void SpectrumController::addSample(std::vector<char> const& rawSample, size_t timestamp)
{
    std::vector<float> sample(sampleSize);

    size_t size = std::min(rawSample.size(), sample.size() * 4);
    mempcpy(sample.data(), rawSample.data(), size);

    addSample(std::move(sample), timestamp);
}

std::vector<float> const& SpectrumController::getLast() const
{
    return samples.get(1);
}
