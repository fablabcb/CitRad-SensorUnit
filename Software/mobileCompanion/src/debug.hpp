#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <chrono>
#include <string>

void printLog(std::string const& str);

struct TimeLog
{
    TimeLog(std::string const& prefix)
        : prefix(prefix)
    {
        creation = std::chrono::steady_clock::now();
        last = creation;
        printLog(prefix + " created");
    }

    void log(std::string const& tag)
    {
        auto const now = std::chrono::steady_clock::now();
        printLog(
            prefix + " " + tag + " " +
            std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(now - creation).count()) + " " +
            std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(now - last).count()));
        last = now;
    }

    std::chrono::steady_clock::time_point creation;
    std::chrono::steady_clock::time_point last;
    std::string const prefix;
};

#endif // DEBUG_HPP
