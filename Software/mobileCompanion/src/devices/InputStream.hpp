#ifndef DEVICES_INPUTSTREAM_HPP
#define DEVICES_INPUTSTREAM_HPP

#include <fstream>
#include <string>
#include <vector>

namespace devices
{

class InputStream
{
  public:
    virtual size_t getBatchSize() const = 0;
    virtual size_t getFftWidth() const = 0;
    virtual size_t getSignalSampleRate() const = 0;
    virtual bool getNextBatch(std::vector<float>& data, size_t& timestamp) = 0;

    virtual ~InputStream() = default;
};

class FileInputStream : public InputStream
{
  public:
    FileInputStream(std::string const& fileName);
    ~FileInputStream() override = default;

  public:
    size_t getBatchSize() const override { return batchSize; }
    size_t getFftWidth() const override { return fftWidth; }
    size_t getSignalSampleRate() const override { return sampleRate; }
    bool getNextBatch(std::vector<float>& data, size_t& timestamp) override;

  private:
    enum class DataType
    {
        UINT8,
        FLOAT
    };
    enum class FormatVersion
    {
        V1,
        V2
    };

  private:
    bool getNextBatch_version1(std::vector<float>& data, size_t& timestamp);
    bool getNextBatch_version2(std::vector<float>& data, size_t& timestamp);

  private:
    size_t batchSize = 0;
    size_t sampleRate = 0;
    size_t fftWidth = 1024;
    std::ifstream file;
    FormatVersion formatVersion = FormatVersion::V1;
    DataType dataType = DataType::UINT8;

    size_t currentBatchNumber = 0;

    std::vector<u_int8_t> buffer;
};

} // namespace devices

#endif
