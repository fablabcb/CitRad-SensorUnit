#include "statistics.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

RunningMean::RunningMean(size_t n, float initialValue)
    : n{n}
    , value(initialValue)
{}

float RunningMean::add(float newValue)
{
    value = 1.0 / n * ((n - 1) * value + newValue);
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

    float const nan = std::numeric_limits<float>::quiet_NaN();
    buffer = RingBuffer(n, nan);
}

float HannWindowSmoothing::add(float newValue)
{
    buffer.add(newValue);

    float result = 0.0f;
    for(size_t i = 0; i < n; i++)
        result += window[i] * buffer.get(i);

    return result / ((n - 1) / 2);
}

float HannWindowSmoothing::get()
{
    return buffer.get(0);
}

float getAlmostMedian(std::vector<float>& data)
{
    if(data.empty())
        return std::numeric_limits<float>::quiet_NaN();

    size_t const index = data.size() / 2;
    std::nth_element(data.begin(), data.begin() + index, data.end());
    return data[index];
}

std::vector<float> filterValid(const std::vector<float>& data)
{
    std::vector<float> result;
    std::copy_if(data.begin(), data.end(), std::back_inserter(result), [](float const& f) {
        return not std::isnan(f);
    });

    return result;
}
