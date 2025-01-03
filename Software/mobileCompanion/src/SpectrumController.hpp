#ifndef SPECTRUMCONTROLLER_HPP
#define SPECTRUMCONTROLLER_HPP

#include "shared/RingBuffer.hpp"

#include <QObject>

#include <vector>

class SpectrumController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int activeSampleIdx READ getActiveSampleIdx NOTIFY activeSampleIdxChanged FINAL)
    Q_PROPERTY(int storedSamplesCount READ getStoredSamplesCount NOTIFY storedSamplesCountChanged FINAL)
    Q_PROPERTY(int maxSampleCount READ getMaxSampleCount WRITE setMaxSampleCount NOTIFY maxSampleCountChanged FINAL)
    Q_PROPERTY(int sampleSize READ getSampleSize CONSTANT FINAL)
    Q_PROPERTY(float speedToBinFactor READ getSpeedToBinFactor CONSTANT FINAL)
    Q_PROPERTY(int sampleRate READ getSampleRate CONSTANT FINAL)

  public:
    SpectrumController(size_t sampleSize, size_t maxSampleCount, size_t sampleRate);

    size_t getActiveSampleIdx() const { return activeSampleIdx; }
    size_t getStoredSamplesCount() const { return storedSamplesCount; }
    size_t getMaxSampleCount() const { return maxSampleCount; }
    size_t getSampleSize() const { return sampleSize; }
    float getSpeedToBinFactor() const { return speedToBinFactor; }
    size_t getSampleRate() const { return sampleRate; }

    void setMaxSampleCount(size_t newMaxSampleCount);

    void addSample(std::vector<float> const& sample, size_t timestamp);
    void addSample(std::vector<float>&& sample, size_t timestamp);
    void addSample(std::vector<char> const& rawSample, size_t timestamp);

    float getLowerValueRange() const { return -256.0f; }
    float getUpperValueRange() const { return 0.0f; }

    std::vector<float> const& getLast() const;
    size_t getLastTimestamp() const { return lastTimestamp; }

  signals:
    void activeSampleIdxChanged();
    void storedSamplesCountChanged();
    void maxSampleCountChanged();
    void sampleAdded();

  private:
    RingBuffer<std::vector<float>> samples;
    size_t activeSampleIdx = 0;
    size_t storedSamplesCount = 0;
    size_t sampleSize = 0;
    size_t maxSampleCount = 0;
    size_t lastTimestamp = 0;
    size_t sampleRate = 0;
    float speedToBinFactor = 0.0f;
};

#endif // SPECTRUMCONTROLLER_HPP
