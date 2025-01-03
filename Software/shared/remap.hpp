#ifndef REMAP_HPP
#define REMAP_HPP

#include <cassert>

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
    _BaseValue<T> fromClamped(T const& lowerBound, T const& upperBound)
    {
        assert(lowerBound < upperBound);
        return _BaseValue<T>{
            value < lowerBound ? lowerBound : (value > upperBound ? upperBound : value),
            lowerBound,
            upperBound};
    }
};

template <class T>
_RemapValue<T> remap(T const& value)
{
    return _RemapValue<T>{value};
}

} // namespace math

#endif
