#include "SpectrumColorizer.hpp"

#include <shared/remap.hpp>

#include <cassert>
#include <cmath>

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

    size_t lower = std::floor(t);
    size_t upper = std::ceil(t);

    return lerp(samplingPoints[lower], samplingPoints[upper], t - lower);
}

void SpectrumColorizer::generateColorsIfRequried(size_t colorCount)
{
    if(colors.size() == colorCount)
        return;

    colors.resize(colorCount);

    std::vector<Color> rawColors = {
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

    for(size_t i = 0; i < colorCount; i++)
    {
        float t = math::remap(1.0 * i).from(0, colorCount - 1).to(0, rawColors.size() - 1);
        colors[i] = lerp(rawColors, t);
    }
}

} // namespace gfx
