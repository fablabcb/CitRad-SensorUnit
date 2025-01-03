#include "InputStream.hpp"

#include <cstdint>
#include <iostream>

auto readNPrint = []<typename T, typename Out = T>(std::ifstream& in, std::string const& name) {
    static T buffer;
    in.read((char*)&buffer, sizeof(T));
    if(not name.empty())
        std::cout << name << ": " << (Out)buffer << std::endl;
    return buffer;
};
auto skip = []<typename T>(std::ifstream& in, std::string const&) {
    static T buffer;
    in.read((char*)&buffer, sizeof(T));
};
auto read = []<typename T, typename Out = T>(std::ifstream& in) {
    static T buffer;
    in.read((char*)&buffer, sizeof(T));
    return buffer;
};

FileInputStream::FileInputStream(std::string const& fileName)
{
    file.open(fileName, std::ios::in | std::ios::binary);
    if(!file)
        throw std::runtime_error("Failed to open " + fileName);

    {
        size_t v = readNPrint.template operator()<uint16_t>(file, "fileFormatVersion");
        if(v == 1)
        {
            formatVersion = FormatVersion::V1;
            fftWidth = 1024;
        }
        else if(v == 2)
        {
            formatVersion = FormatVersion::V2;
            fftWidth = 1024;
        }
        else
            throw std::runtime_error("Unknown file format version " + std::to_string(v));
    }

    readNPrint.template operator()<uint32_t>(file, "timestamp");
    batchSize = readNPrint.template operator()<uint16_t>(file, "binCount");
    if(formatVersion == FormatVersion::V2)
    {
        size_t dataSize = 1;
        dataSize = readNPrint.template operator()<u_int8_t, int>(file, "dataSize");
        if(dataSize == 1)
        {
            dataType = DataType::UINT8;
            buffer.resize(batchSize);
        }
        else
        {
            dataType = DataType::FLOAT;
        }
    }
    else if(formatVersion == FormatVersion::V1)
    {
        dataType = DataType::UINT8;
        buffer.resize(batchSize);
    }

    readNPrint.template operator()<bool>(file, "isIQ");
    sampleRate = readNPrint.template operator()<uint16_t>(file, "sampleRate");
}

bool FileInputStream::getNextBatch(std::vector<float>& data, size_t& timestamp)
{
    if(file.eof())
    {
        std::cerr << "Reached end of file" << std::endl;
        return false;
    }
    if(data.size() != batchSize)
        throw std::runtime_error("data array for getNextBatch has wrong size");

    switch(formatVersion)
    {
        case FormatVersion::V1:
            return getNextBatch_version1(data, timestamp);
        case FormatVersion::V2:
            return getNextBatch_version2(data, timestamp);
    }
    // should never reach this
    return false;
}

bool FileInputStream::getNextBatch_version1(std::vector<float>& data, size_t& timestamp)
{
    timestamp = read.template operator()<uint32_t>(file);

    switch(dataType)
    {
        case DataType::UINT8:
            file.read((char*)buffer.data(), batchSize);
            if(file.eof() && file.fail())
                return false;

            for(size_t i = 0; i < batchSize; i++)
                data[i] = -buffer[i];
            break;

        case DataType::FLOAT:
            file.read((char*)data.data(), batchSize * 4);
            if(file.eof() && file.fail())
                return false;
            break;
    }

    return true;
}

bool FileInputStream::getNextBatch_version2(std::vector<float>& data, size_t& timestamp)
{
    timestamp = read.template operator()<uint32_t>(file);

    auto binCount = read.template operator()<u_int32_t>(file);
    if(file.eof() && file.fail())
        return false;

    if(batchSize != binCount)
    {
        std::cerr << "Bin count does not match in batch " << currentBatchNumber << std::endl;
        return false;
    }

    switch(dataType)
    {
        case DataType::UINT8:
            file.read((char*)buffer.data(), batchSize);
            if(file.eof() && file.fail())
                return false;

            for(size_t i = 0; i < batchSize; i++)
                data[i] = -(int)buffer[i];
            break;

        case DataType::FLOAT:
            file.read((char*)data.data(), batchSize * 4);
            if(file.eof() && file.fail())
                return false;
            break;
    }

    uint32_t endMarker = read.template operator()<u_int32_t>(file);
    if(~endMarker != 0)
    {
        std::cerr << "Invalid End Marker in batch " << currentBatchNumber << std::endl;
        return false;
    }

    currentBatchNumber++;

    return true;
}
