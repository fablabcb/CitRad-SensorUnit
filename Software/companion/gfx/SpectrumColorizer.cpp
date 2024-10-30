#include "SpectrumColorizer.hpp"

#include "../../sensor/statistics.hpp"

#include <cassert>
#include <cmath>
#include <stdexcept>

namespace gfx
{
template <class V>
V lerp(V const& a, V const& b, float t)
{
    assert(a.size() == b.size());

    V result = a;
    for(size_t i = 0; i < result.size(); i++)
        result[i] = a[i] + (b[i] - a[i]) * t;

    return result;
}

/**
 *  Function to get a lerp value out of a vector of numbers. The valid range for t is [0;numOfElements-1]
 */
template <class V>
V lerp(std::vector<V> const& samplingPoints, float t)
{
    if(t <= 0.0)
        return samplingPoints.at(0);
    if(t >= samplingPoints.size())
        return samplingPoints.back();

    // 01234
    // ABCDE
    // 3.2
    size_t lower = std::floor(t);
    size_t upper = std::ceil(t);

    return lerp(samplingPoints[lower], samplingPoints[upper], t - lower);
}

void SpectrumColorizer::ColorRingbuffer::advance()
{
    base += 1;
    if(base == colors.size())
        base = 0;
}

SpectrumColorizer::ColorRingbuffer::Color const& SpectrumColorizer::ColorRingbuffer::get(size_t index) const
{
    if(colors.empty())
        throw std::runtime_error("Empty color vector");

    auto idx = base + index;
    while(idx >= colors.size())
        idx -= colors.size();

    return colors[idx];
}

void SpectrumColorizer::ColorRingbuffer::setSize(size_t size)
{
    if(colors.size() == size)
        return;

    base = 0;
    colors.resize(size);

    std::vector<std::array<float, 3>> rawColors = {
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        //{0, 0, 0},
        //{0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {1, 1, 0},
        //{0, 1, 0},
        {1, 0, 0},
        {1, 1, 1},
        {0, 0, 1},
        {0, 0, 1},
        {0, 0, 1}};

    for(size_t i = 0; i < size; i++)
    {
        float t = math::remap(1.0 * i).from(0, size - 1).to(0, rawColors.size() - 1);
        colors[i] = lerp(rawColors, t);
    }
}

void SpectrumColorizer::generateColorsIfRequried(size_t colorCount)
{
    colorBuffer.setSize(colorCount);
}

} // namespace gfx
