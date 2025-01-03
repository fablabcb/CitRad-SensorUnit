#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE shared::FiFo

#include <FiFo.hpp>

#include <boost/test/unit_test.hpp>

#include <array>

BOOST_AUTO_TEST_SUITE(FiFoTestSuite)

BOOST_AUTO_TEST_CASE(constructor_ok)
{
    BOOST_CHECK_THROW(FiFo<int> fifo(0), std::invalid_argument);
    BOOST_CHECK_NO_THROW(FiFo<int> fifo(1));
    BOOST_CHECK_NO_THROW(FiFo<int> fifo(10));

    FiFo<int> fifo(10);
    IMPL::FifoInspector<int> insp(fifo);
    BOOST_CHECK_EQUAL(fifo.size(), 10);
    BOOST_CHECK_EQUAL(insp.readIdx(), 0);
    BOOST_CHECK_EQUAL(insp.writeIdx(), 0);

    fifo = FiFo<int>(1);
    BOOST_CHECK_EQUAL(fifo.size(), 1);
}

BOOST_AUTO_TEST_CASE(push_working)
{
    FiFo<int> fifo(5);
    IMPL::FifoInspector<int> insp(fifo);

    BOOST_CHECK_EQUAL(fifo.getAvailableCount(), 0);
    fifo.push(1);
    BOOST_CHECK_EQUAL(insp.writeIdx(), 1);
    BOOST_CHECK_EQUAL(fifo.getAvailableCount(), 1);
    fifo.push(2);
    fifo.push(3);
    fifo.push(4);
    fifo.push(5);
    BOOST_CHECK_EQUAL(insp.writeIdx(), 0);
    BOOST_CHECK_EQUAL(fifo.getAvailableCount(), 5);
    fifo.push(6);
    BOOST_CHECK_EQUAL(insp.writeIdx(), 1);
    fifo.pop();
    BOOST_CHECK_EQUAL(fifo.getAvailableCount(), 4);
    fifo.push(6);
    fifo.push(6);
    BOOST_CHECK_EQUAL(fifo.getAvailableCount(), 5);
}

BOOST_AUTO_TEST_CASE(pop_working)
{
    FiFo<int> fifo(5);
    IMPL::FifoInspector<int> insp(fifo);

    fifo.push(1);
    fifo.push(2);
    fifo.push(3);
    fifo.push(4);
    fifo.push(5);
    fifo.push(6);

    BOOST_CHECK_EQUAL(insp.readIdx(), 1);
    BOOST_CHECK_EQUAL(insp.writeIdx(), 1);

    BOOST_CHECK_EQUAL(fifo.getAvailableCount(), 5);
    BOOST_CHECK_EQUAL(fifo.pop(), 2);
    BOOST_CHECK_EQUAL(insp.readIdx(), 2);
    BOOST_CHECK_EQUAL(fifo.getAvailableCount(), 4);
    BOOST_CHECK_EQUAL(fifo.pop(), 3);
    BOOST_CHECK_EQUAL(insp.readIdx(), 3);
    BOOST_CHECK_EQUAL(fifo.getAvailableCount(), 3);
    BOOST_CHECK_EQUAL(fifo.pop(), 4);
    BOOST_CHECK_EQUAL(insp.readIdx(), 4);
    BOOST_CHECK_EQUAL(fifo.getAvailableCount(), 2);
    BOOST_CHECK_EQUAL(fifo.pop(), 5);
    BOOST_CHECK_EQUAL(insp.readIdx(), 0);
    BOOST_CHECK_EQUAL(fifo.getAvailableCount(), 1);
    BOOST_CHECK_EQUAL(fifo.pop(), 6);
    BOOST_CHECK_EQUAL(insp.readIdx(), 1);
    BOOST_CHECK_EQUAL(fifo.getAvailableCount(), 0);
}

BOOST_AUTO_TEST_CASE(write_checksArguments)
{
    int buffer[10];
    FiFo<int> fifo(5);
    BOOST_CHECK_NO_THROW(fifo.write(buffer, 5));
    BOOST_CHECK_NO_THROW(fifo.write(buffer, 0));
    BOOST_CHECK_THROW(fifo.write(buffer, 6), std::invalid_argument);
    BOOST_CHECK_THROW(fifo.write(nullptr, 0), std::invalid_argument);
    BOOST_CHECK_THROW(fifo.write(nullptr, 6), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(read_checksArguments)
{
    int buffer[10];
    FiFo<int> fifo(5);
    BOOST_CHECK_NO_THROW(fifo.read(buffer, 5));
    BOOST_CHECK_NO_THROW(fifo.read(buffer, 0));
    BOOST_CHECK_THROW(fifo.read(buffer, 6), std::invalid_argument);
    BOOST_CHECK_THROW(fifo.read(nullptr, 0), std::invalid_argument);
    BOOST_CHECK_THROW(fifo.read(nullptr, 6), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(write_working)
{
    std::array<int, 10> buffer = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    FiFo<int> fifo(5);
    IMPL::FifoInspector<int> insp(fifo);

    BOOST_CHECK_EQUAL(fifo.getAvailableCount(), 0);
    fifo.write(buffer.data(), 0);
    BOOST_CHECK_EQUAL(insp.writeIdx(), 0);
    BOOST_CHECK_EQUAL(fifo.getAvailableCount(), 0);

    fifo.write(buffer.data(), 1); // now: [r1,1,,,,]
    BOOST_CHECK_EQUAL(insp.writeIdx(), 1);
    BOOST_CHECK_EQUAL(fifo.getAvailableCount(), 1);
    BOOST_CHECK_EQUAL(fifo.pop(), 1); // now: [,rw,,,,]
    BOOST_CHECK_EQUAL(insp.readIdx(), 1);

    fifo.write(buffer.data(), 5); // now: [5,rw1,2,3,4]+full
    BOOST_CHECK_EQUAL(insp.writeIdx(), 1);
    BOOST_CHECK_EQUAL(insp.readIdx(), insp.writeIdx());
    BOOST_CHECK_EQUAL(fifo.getAvailableCount(), 5);
    BOOST_CHECK_EQUAL(fifo.pop(), 1); // now: [5,w1,r2,3,4]
    BOOST_CHECK_EQUAL(insp.readIdx(), 2);

    fifo.write(buffer.data() + 5, 2); // overwrites first item and moves readIdx
    BOOST_CHECK_EQUAL(insp.writeIdx(), 3);
    BOOST_CHECK_EQUAL(insp.readIdx(), 3);
    BOOST_CHECK_EQUAL(fifo.getAvailableCount(), 5); // now: [5,6,7,rw3,4]
    BOOST_CHECK_EQUAL(fifo.pop(), 3);
    BOOST_CHECK_EQUAL(fifo.pop(), 4);
    BOOST_CHECK_EQUAL(fifo.pop(), 5);
    BOOST_CHECK_EQUAL(fifo.pop(), 6);
    BOOST_CHECK_EQUAL(fifo.pop(), 7);
    BOOST_CHECK_EQUAL(insp.writeIdx(), 3);
    BOOST_CHECK_EQUAL(insp.readIdx(), 3);
    BOOST_CHECK_EQUAL(fifo.getAvailableCount(), 0);
}

BOOST_AUTO_TEST_CASE(read_working)
{
    std::array<int, 10> writeBuffer = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::array<int, 5> readBuffer{};
    std::array<int, 5> test = {1, 2, 3, 4, 5};
    FiFo<int> fifo(5);
    IMPL::FifoInspector<int> insp(fifo);

    BOOST_CHECK_EQUAL(fifo.getAvailableCount(), 0);
    fifo.write(writeBuffer.data(), 5);
    BOOST_CHECK_EQUAL(insp.writeIdx(), 0);
    BOOST_CHECK_EQUAL(insp.readIdx(), 0);
    BOOST_CHECK_EQUAL(fifo.getAvailableCount(), 5);

    fifo.read(readBuffer.data(), 1);
    BOOST_CHECK_EQUAL(insp.readIdx(), 1);
    BOOST_CHECK_EQUAL(readBuffer[0], 1);
    BOOST_CHECK_EQUAL(fifo.getAvailableCount(), 4);

    fifo.read(readBuffer.data(), 1);
    BOOST_CHECK_EQUAL(insp.readIdx(), 2);
    BOOST_CHECK_EQUAL(readBuffer[0], 2);
    BOOST_CHECK_EQUAL(fifo.getAvailableCount(), 3);

    fifo.read(readBuffer.data(), 3);
    BOOST_CHECK_EQUAL(insp.readIdx(), 0);
    BOOST_CHECK_EQUAL(readBuffer[0], 3);
    BOOST_CHECK_EQUAL(readBuffer[1], 4);
    BOOST_CHECK_EQUAL(readBuffer[2], 5);
    BOOST_CHECK_EQUAL(fifo.getAvailableCount(), 0);

    fifo.write(writeBuffer.data(), 5);
    fifo.read(readBuffer.data(), 5);
    BOOST_TEST(readBuffer == test);

    fifo = FiFo<int>(5);
    fifo.write(writeBuffer.data(), 1);
    fifo.write(writeBuffer.data(), 5);
    fifo.read(readBuffer.data(), 2);
    BOOST_TEST(readBuffer == test);
}

BOOST_AUTO_TEST_CASE(peek_checksArguments)
{
    int buffer[10];
    FiFo<int> fifo(5);
    BOOST_CHECK_NO_THROW(fifo.peek(buffer, 5));
    BOOST_CHECK_NO_THROW(fifo.peek(buffer, 0));
    BOOST_CHECK_THROW(fifo.peek(buffer, 6), std::invalid_argument);
    BOOST_CHECK_THROW(fifo.peek(nullptr, 0), std::invalid_argument);
    BOOST_CHECK_THROW(fifo.peek(nullptr, 6), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(peak_working)
{
    std::array<int, 10> writeBuffer = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::array<int, 5> readBuffer{};
    std::array<int, 5> test = {1, 2, 3, 4, 5};
    FiFo<int> fifo(5);

    BOOST_CHECK_EQUAL(fifo.getAvailableCount(), 0);
    fifo.write(writeBuffer.data(), 5);
    BOOST_CHECK_EQUAL(fifo.getAvailableCount(), 5);

    fifo.peek(readBuffer.data(), 1);
    BOOST_CHECK_EQUAL(readBuffer[0], 1);
    fifo.peek(readBuffer.data(), 1);
    BOOST_CHECK_EQUAL(readBuffer[0], 1);

    fifo.peek(readBuffer.data(), 3);
    BOOST_CHECK_EQUAL(readBuffer[0], 1);
    BOOST_CHECK_EQUAL(readBuffer[1], 2);
    BOOST_CHECK_EQUAL(readBuffer[2], 3);
    BOOST_CHECK_EQUAL(fifo.getAvailableCount(), 5);

    fifo.write(writeBuffer.data(), 5);
    fifo.peek(readBuffer.data(), 5);
    BOOST_TEST(readBuffer == test);

    fifo = FiFo<int>(5);
    fifo.write(writeBuffer.data(), 1);
    fifo.write(writeBuffer.data(), 5);
    fifo.peek(readBuffer.data(), 2);
    BOOST_TEST(readBuffer == test);
}

BOOST_AUTO_TEST_CASE(skip_working)
{
    std::array<int, 10> writeBuffer = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    FiFo<int> fifo(5);
    IMPL::FifoInspector<int> insp(fifo);

    fifo.write(writeBuffer.data(), 5);
    BOOST_CHECK_EQUAL(insp.writeIdx(), 0);
    BOOST_CHECK_EQUAL(insp.readIdx(), 0);

    BOOST_CHECK_EQUAL(fifo.skip(0), 0);
    BOOST_CHECK_EQUAL(insp.readIdx(), 0);

    BOOST_CHECK_EQUAL(fifo.skip(1), 1);
    BOOST_CHECK_EQUAL(insp.readIdx(), 1);

    BOOST_CHECK_EQUAL(fifo.skip(4), 4);
    BOOST_CHECK_EQUAL(insp.readIdx(), 0);

    BOOST_CHECK_EQUAL(fifo.skip(4), 0);
    BOOST_CHECK_EQUAL(insp.readIdx(), 0);

    fifo.write(writeBuffer.data(), 5);
    BOOST_CHECK_EQUAL(fifo.skip(3), 3);
    BOOST_CHECK_EQUAL(insp.readIdx(), 3);

    fifo.write(writeBuffer.data(), 5);
    BOOST_CHECK_EQUAL(fifo.skip(4), 4);
    BOOST_CHECK_EQUAL(insp.readIdx(), 4);
}

BOOST_AUTO_TEST_SUITE_END();
