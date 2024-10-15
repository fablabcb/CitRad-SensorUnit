#ifndef STATISTICS_HPP
#define STATISTICS_HPP

#include <cstddef>
#include <vector>

template <class T>
class RingBuffer
{
  public:
    RingBuffer() = default;
    RingBuffer(size_t size, T const& initialValue)
    {
        if(size % 2 == 0)
            size += 1;

        buffer.resize(size, initialValue);
    }

    T get(size_t offset) const { return buffer[(index + offset) % buffer.size()]; }

    void set(T value) { buffer[index] = value; }
    void add(T value)
    {
        set(value);
        incrementIndex();
    }

    void incrementIndex()
    {
        if(index + 1 >= buffer.size())
            index = 0;
        else
            index += 1;
    }

    std::vector<T> getLastN(size_t count)
    {
        std::vector<T> result(std::min(count, buffer.size()));
        for(size_t i = 0; i < count; i++)
            result[i] = get(i);
        return result;
    }

  private:
    std::vector<T> buffer;
    size_t index = 0;
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

  private:
    size_t n;
    std::vector<float> window;
    RingBuffer<float> buffer;
};

// not exactly median since we do not average out 2 values if the number of items is even
float getAlmostMedian(std::vector<float>& data);

std::vector<float> filterValid(std::vector<float> const& data);

#endif
