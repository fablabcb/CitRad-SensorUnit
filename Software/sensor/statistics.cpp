#include "statistics.hpp"

#include <cmath>
#include <limits>

RunningMean::RunningMean(size_t n, float initialValue)
    : n{n}
    , value(initialValue)
{}

float RunningMean::add(float newValue)
{
    value = 1 / n * ((n - 1) * value + newValue);
    return value;
}

HannWindowSmoothing::HannWindowSmoothing(size_t size)
    : n(size)
{
    if(n % 2 == 0)
        n += 1;

    window.resize(n);
    for(size_t i = 0; i < n; i++)
        window[i] = 0.5 - 0.5 * cos(2 * M_PI * i / (n - 1));

    buffer = RingBuffer(n);
}

float HannWindowSmoothing::add(float newValue)
{
    buffer.set(newValue);
    buffer.incrementIndex();

    float result = 0.0f;
    for(int i = 0; i < n; i++)
        result += window[i] * buffer.get(i);

    return result / ((n - 1) / 2);
}

RingBuffer::RingBuffer(size_t size)
{
    if(size % 2 == 0)
        size += 1;

    buffer.resize(size);
}

float RingBuffer::get(size_t offset) const
{
    return buffer[(index + offset) % buffer.size()];
}

void RingBuffer::set(float value)
{
    buffer[index] = value;
}

void RingBuffer::incrementIndex()
{
    if(index + 1 >= buffer.size())
        index = 0;
    else
        index += 1;
}
