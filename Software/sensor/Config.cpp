#include "Config.hpp"

#include <charconv>
#include <functional>
#include <vector>

template <class T>
void run(
    std::function<bool(std::string const&, T&)> fnc,
    std::string const& key,
    T& result,
    std::map<std::string, std::string>& map)
{
    auto const it = map.find(key);
    if(it == map.end())
        return;

    bool ok = fnc(it->second, result);
    if(ok == true)
        Serial.println(String("(D) Overwriting ") + String(key.c_str()));
    else
        Serial.println(String("(E) Failed to read config value ") + String(key.c_str()));

    map.erase(it);
}

void Config::process(std::map<std::string, std::string> const& map)
{
    auto parseBool = [](std::string const& value, bool& result) -> bool {
        if(value == "true")
        {
            result = true;
            return true;
        }
        if(value == "false")
        {
            result = false;
            return true;
        }
        return false;
    };
    auto parseSize = [](std::string const& value, size_t& result) -> bool {
        auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), result);
        return ec == std::errc{};
    };
    auto parseString = [](std::string const& value, std::string& result) -> bool {
        result = value;
        return true;
    };

    auto mapCpy = map;
    run<bool>(parseBool, "writeDataToSdCard", this->writeDataToSdCard, mapCpy);
    run<bool>(parseBool, "write8bit", this->write8bit, mapCpy);
    run<bool>(parseBool, "writeRawData", this->writeRawData, mapCpy);
    run<bool>(parseBool, "writeCsvData", this->writeCsvData, mapCpy);
    run<bool>(parseBool, "splitLargeFiles", this->splitLargeFiles, mapCpy);

    run<size_t>(parseSize, "maxSecondsPerFile", this->maxSecondsPerFile, mapCpy);
    run<std::string>(parseString, "filePrefix", this->filePrefix, mapCpy);

    if(mapCpy.size() > 0)
    {
        Serial.println(String("(E) Config contains unknown values"));
        for(auto const& [key, value] : mapCpy)
            Serial.println(String("(D) ") + String(key.c_str()));
    }
}