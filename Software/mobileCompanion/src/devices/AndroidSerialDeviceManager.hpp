#ifndef DEVICES_ANDROIDSERIALDEVICEMANAGER_HPP
#define DEVICES_ANDROIDSERIALDEVICEMANAGER_HPP

// #define ANDROID

#ifdef ANDROID

#include "IDeviceManager.hpp"

#include <shared/FiFo.hpp>

#include <QString>
#include <QtAndroidExtras/QtAndroid>

namespace devices
{

class AndroidSerialConnection : public IConnection
{
  public:
    AndroidSerialConnection(size_t deviceId, std::string const& name, IConnection::Mode mode);

    bool hasError() const override { return hadError; }
    bool isConnected() const override;
    void disconnect() override;

    size_t skip(size_t maxLength) override;
    size_t peek(char* buffer, size_t maxLength) override;
    size_t read(char* buffer, size_t maxLength) override;
    size_t readLine(char* buffer, size_t maxLength) override;

  private:
    size_t getAvailableAndFetch(size_t requestedLength);
    void readNextBatch();

  private:
    bool isOpen = false;
    bool hadError = false;
    QAndroidJniObject serialObject;
    FiFo<char> fifo;
};

class AndroidSerialDeviceManager : public IDeviceManager
{
  public:
    AndroidSerialDeviceManager();

    std::vector<std::string> getDeviceNames() override;
    std::unique_ptr<IConnection> connect(std::string const& name, IConnection::Mode mode) override;

  private:
    int getUsbDeviceCount();
    QString getName(size_t deviceIndex);

  private:
    QAndroidJniObject serialObject;
};

} // namespace devices

#endif
#endif
