#ifndef STATISTICS_HPP
#define STATISTICS_HPP

#include <cassert>
#include <cstddef>
#include <vector>

template <class T>
struct Range
{
    T from;
    T to;
};

namespace math
{

// usage
// float value = 2.5;
// float mappedValue = math::remap(value).from(0,10).to(0,100);
template <class T>
struct _BaseValue
{
    T value;
    T lowerBound;
    T upperBound;

    T to(T const& newLowerBound, T const& newUpperBound)
    {
        if(value != value)
            return value;

        double f = 1.0 * (value - lowerBound) / (upperBound - lowerBound);
        return newLowerBound + f * (newUpperBound - newLowerBound);
    }
};

template <class T>
struct _RemapValue
{
    T value;

    _BaseValue<T> from(T const& lowerBound, T const& upperBound)
    {
        assert(value != value || lowerBound <= value);
        assert(value != value || value <= upperBound);
        return _BaseValue<T>{value, lowerBound, upperBound};
    }
};

template <class T>
_RemapValue<T> remap(T const& value)
{
    return _RemapValue<T>{value};
}

} // namespace math

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

    T get(size_t offset) const
    {
        size_t cleanOffset = offset % buffer.size();
        if(cleanOffset <= index)
            return buffer[index - cleanOffset];

        return buffer[index + buffer.size() - cleanOffset];
    }

    void set_noIncrement(T value) { buffer[index] = value; }
    void add(T value)
    {
        set_noIncrement(value);
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
        assert(count <= buffer.size());

        std::vector<T> result(std::min(count, buffer.size()));
        for(size_t i = 0; i < result.size(); i++)
            result[count - 1 - i] = get(i);
        return result;
    }

    std::vector<T> getLastNButSkipFirst(size_t count, size_t skipCount)
    {
        assert(count <= buffer.size());

        std::vector<T> result;
        result.reserve(std::min(count - skipCount, buffer.size()));
        for(size_t i = skipCount; i < std::min(buffer.size(), count); i++)
            result.push_back(get(i));
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
