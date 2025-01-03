#ifndef STATISTICS_HPP
#define STATISTICS_HPP

#include "RingBuffer.hpp"

#include <cassert>
#include <cstddef>
#include <vector>

template <class T>
struct Range
{
    T from;
    T to;
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

    // add new value
    float add(float newValue);

    // get current value
    float get();

    size_t getWindowWidth() const { return n; }

  private:
    size_t n;
    float currentValue = 0;
    std::vector<float> window;
    RingBuffer<float> buffer;
};

// not exactly median since we do not average out 2 values if the number of items is even
float getAlmostMedian(std::vector<float>& data);

std::vector<float> filterValid(std::vector<float> const& data);

#endif
