#ifndef DEVICES_IDEVICEMANAGER_HPP
#define DEVICES_IDEVICEMANAGER_HPP

#include <memory>
#include <string>
#include <vector>

class QThread;

namespace devices
{

class IConnection
{
  public:
    enum class Mode
    {
        R,
        W,
        RW
    };

    virtual bool isConnected() const = 0;
    virtual void disconnect() = 0;

  public:
    IConnection(std::string const& name, Mode mode)
        : name{name}
        , mode{mode}
    {}
    virtual ~IConnection() = default;
    std::string getName() const { return name; }
    Mode getMode() const { return mode; }

    virtual bool hasError() const = 0;
    virtual size_t skip(size_t maxLength) = 0;
    virtual size_t peek(char* buffer, size_t maxLength) = 0;
    virtual size_t read(char* buffer, size_t maxLength) = 0;
    virtual size_t readLine(char* buffer, size_t maxLength) = 0;

  private:
    std::string const name;
    Mode const mode;
};

class IDeviceManager
{
  public:
    virtual ~IDeviceManager() = default;

    virtual std::vector<std::string> getDeviceNames() = 0;

    virtual std::unique_ptr<IConnection> connect(std::string const& name, IConnection::Mode mode) = 0;
};

} // namespace devices

#endif
