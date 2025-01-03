#ifndef FIFO_HPP
#define FIFO_HPP

#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

/*
#ifdef ANDROID
#include <android/log.h>
#else
#include <iostream>
#endif
    void log(std::string const& str)
    {
#ifdef ANDROID
        __android_log_write(ANDROID_LOG_DEBUG, "Serial FIFO", str.c_str());
#else
        std::cout << "FiFo: " << str << std::endl;
#endif
    }
*/

namespace IMPL
{
template <class T>
class FifoInspector;
}

template <class T>
class FiFo
{
  private:
    friend class IMPL::FifoInspector<T>;

  public:
    FiFo(size_t size)
    {
        if(size == 0)
            throw std::invalid_argument("FiFo cannot be empty.");
        buffer.resize(size);
    }

    size_t size() const { return buffer.size(); }

    void push(T const& v)
    {
        buffer[writeIdx] = v;
        writeIdx = (writeIdx + 1 == buffer.size()) ? 0 : writeIdx + 1;
        if(isFull)
            readIdx = writeIdx;
        else if(writeIdx == readIdx)
            isFull = true;
    }

    T const& pop()
    {
        auto const& result = buffer[readIdx];
        readIdx = (readIdx + 1 == buffer.size()) ? 0 : readIdx + 1;
        isFull = false;
        return result;
    }

    void write(T const* data, size_t count)
    {
        if(data == nullptr)
            throw std::invalid_argument("FiFo::write(nullptr): cannot write from nullptr");
        if(count == 0)
            return;

        if(count > buffer.size())
            throw std::invalid_argument(
                "FiFo::write(data," + std::to_string(count) + "): exceeds buffer size of " +
                std::to_string(buffer.size()));

        if(getAvailableCount() + count >= buffer.size())
            isFull = true;

        size_t batchSize1 = std::min(count, buffer.size() - writeIdx);
        std::memcpy(buffer.data() + writeIdx, data, sizeof(T) * batchSize1);
        writeIdx = (writeIdx + batchSize1 == buffer.size()) ? 0 : writeIdx + batchSize1;

        size_t batchSize2 = count - batchSize1;
        if(batchSize2 > 0)
        {
            std::memcpy(buffer.data() + writeIdx, data + batchSize1, sizeof(T) * batchSize2);
            writeIdx += batchSize2;
        }

        if(isFull)
            readIdx = writeIdx;
    }

    void read(T* data, size_t count)
    {
        if(data == nullptr)
            throw std::invalid_argument("FiFo::read(nullptr): cannot read to nullptr");
        if(count == 0)
            return;

        if(count > buffer.size())
            throw std::invalid_argument(
                "FiFo::read(data," + std::to_string(count) + "): exceeds buffer size of " +
                std::to_string(buffer.size()));

        isFull = false;

        size_t batchSize1 = std::min(count, buffer.size() - readIdx);
        std::memcpy(data, buffer.data() + readIdx, sizeof(T) * batchSize1);
        readIdx = (readIdx + batchSize1 == buffer.size()) ? 0 : readIdx + batchSize1;

        size_t batchSize2 = count - batchSize1;
        if(batchSize2 > 0)
        {
            std::memcpy(data + batchSize1, buffer.data() + readIdx, sizeof(T) * batchSize2);
            readIdx += batchSize2;
        }
    }

    size_t peek(T* data, size_t count)
    {
        if(data == nullptr)
            throw std::invalid_argument("FiFo::peek(nullptr): cannot peek to nullptr");
        if(count == 0)
            return 0;

        if(count > buffer.size())
            throw std::invalid_argument(
                "FiFo::peek(data," + std::to_string(count) + "): exceeds buffer size of " +
                std::to_string(buffer.size()));

        count = std::min(getAvailableCount(), count);
        size_t batchSize1 = std::min(count, buffer.size() - readIdx);
        std::memcpy(data, buffer.data() + readIdx, sizeof(T) * batchSize1);
        size_t fakeReadIdx = (readIdx + batchSize1 == buffer.size()) ? 0 : readIdx + batchSize1;

        size_t batchSize2 = count - batchSize1;
        if(batchSize2 > 0)
            std::memcpy(data + batchSize1, buffer.data() + fakeReadIdx, sizeof(T) * batchSize2);

        return count;
    }

    size_t skip(size_t count)
    {
        size_t available = getAvailableCount();

        if(count == 0 || available == 0)
            return 0;

        count = std::min(available, count);
        isFull = false;
        readIdx = (readIdx + count >= buffer.size()) ? count + readIdx - buffer.size() : readIdx + count;
        return count;
    }

    size_t getAvailableCount() const
    {
        if(isFull)
            return buffer.size();

        if(writeIdx >= readIdx)
            return writeIdx - readIdx;

        return buffer.size() - readIdx + writeIdx;
    }

  private:
    std::vector<T> buffer;
    size_t readIdx = 0;
    size_t writeIdx = 0;
    bool isFull = false;
};

namespace IMPL
{
template <class T>
class FifoInspector
{
  public:
    FifoInspector(FiFo<T> const& fifo)
        : fifo(fifo)
    {}

    size_t readIdx() { return fifo.readIdx; }
    size_t writeIdx() { return fifo.writeIdx; }

    FiFo<T> const& fifo;
};
} // namespace IMPL

#endif
