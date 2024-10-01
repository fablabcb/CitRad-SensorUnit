#ifndef STATISTICS_HPP
#define STATISTICS_HPP

#include <cstddef>
#include <vector>

class RingBuffer
{
  public:
    RingBuffer() = default;
    RingBuffer(size_t size);

    float get(size_t offset) const;
    void set(float value);
    void incrementIndex();

  private:
    std::vector<float> buffer;
    signed int index = 0;
};

class RunningMean
{
  public:
    RunningMean() = default;
    RunningMean(size_t n, float initialValue);

    float add(float newValue);

  private:
    size_t n = 100;
    float value = 0.0f;
};

class HannWindowSmoothing
{
  public:
    HannWindowSmoothing() = default;
    HannWindowSmoothing(size_t n);

    float add(float newValue);

  private:
    size_t n;
    std::vector<float> window;
    RingBuffer buffer;
};

#endif
