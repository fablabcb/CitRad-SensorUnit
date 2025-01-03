#ifndef RINGBUFFER_HPP
#define RINGBUFFER_HPP

#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

template <class T>
class RingBuffer
{
  public:
    RingBuffer() = default;
    RingBuffer(size_t size) { buffer.resize(size); }
    RingBuffer(size_t size, T const& initialValue) { buffer.resize(size, initialValue); }

    size_t size() const { return buffer.size(); }

    T const& get(size_t offset) const
    {
        size_t cleanOffset = offset % buffer.size();
        if(cleanOffset <= index)
            return buffer[index - cleanOffset];

        return buffer[index + buffer.size() - cleanOffset];
    }

    void set_noIncrement(T const& value) { buffer[index] = value; }

    void add(T const& value)
    {
        buffer[index] = value;
        incrementIndex();
    }

    void add(T&& value)
    {
        std::swap(buffer[index], value);
        incrementIndex();
    }

    void incrementIndex()
    {
        if(index + 1 >= buffer.size())
            index = 0;
        else
            index += 1;
    }

    // copies the last 'count' items to the output field
    void copy(T* output, size_t count)
    {
        if(count == 0)
            return;

        if(count > buffer.size())
            throw std::invalid_argument("RingBuffer::copy(output," + std::to_string(count) + "): exceeds buffer size");

        size_t batch1Size = std::min(count, index);
        size_t batch2Size = count > index ? 0 : count - index;

        memcpy(output + batch2Size, buffer.data() + index - batch1Size, sizeof(T) * batch1Size);
        if(batch2Size > 0)
            memcpy(output, buffer.data() + buffer.size() - batch2Size, sizeof(T) * batch2Size);
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

#endif
