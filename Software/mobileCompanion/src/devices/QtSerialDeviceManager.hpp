#ifndef DEVICES_QTSERIALDEVICEMANAGER_HPP
#define DEVICES_QTSERIALDEVICEMANAGER_HPP

#ifndef ANDROID

#include "IDeviceManager.hpp"

#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

namespace devices
{

class QtSerialConnection : public IConnection
{
  public:
    QtSerialConnection(std::string const& name, QSerialPortInfo const& info, IConnection::Mode mode);

    bool hasError() const override;
    bool isConnected() const override;
    void disconnect() override;

    size_t skip(size_t maxLength) override;
    size_t peek(char* buffer, size_t maxLength) override;
    size_t read(char* buffer, size_t maxLength) override;
    size_t readLine(char* buffer, size_t maxLength) override;

    QSerialPort serialPort;

  private:
    bool hadError = false;
    void checkState();
};

class QtSerialDeviceManager : public IDeviceManager
{
  public:
    std::vector<std::string> getDeviceNames() override;

    std::unique_ptr<IConnection> connect(std::string const& name, IConnection::Mode mode) override;
};

} // namespace devices

#endif
#endif
