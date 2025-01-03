#include "QtSerialDeviceManager.hpp"

#ifndef ANDROID

#include <QThread>
#include <iostream>

namespace devices
{

std::vector<std::string> QtSerialDeviceManager::getDeviceNames()
{
    std::vector<std::string> result;

    for(QSerialPortInfo const& info : QSerialPortInfo::availablePorts())
    {
        if(info.description().isEmpty())
            continue;

        result.push_back((info.portName() + "#" + info.description()).toStdString());
    }

    return result;
}

std::unique_ptr<IConnection> QtSerialDeviceManager::connect(std::string const& name, IConnection::Mode mode)
{
    for(QSerialPortInfo const& info : QSerialPortInfo::availablePorts())
    {
        if(info.description().isEmpty())
            continue;

        auto testName = (info.portName() + "#" + info.description()).toStdString();
        if(testName == name)
        {
            return std::make_unique<QtSerialConnection>(name, info, mode);
        }
    }
    return {};
}

QtSerialConnection::QtSerialConnection(std::string const& name, QSerialPortInfo const& info, IConnection::Mode mode)
    : IConnection(name, mode)
{
    serialPort.setPort(info);
    serialPort.open(
        mode == Mode::R ? QIODevice::ReadOnly : (mode == Mode::W ? QIODevice::WriteOnly : QIODevice::ReadWrite));
}

bool QtSerialConnection::hasError() const
{
    return hadError;
}

bool QtSerialConnection::isConnected() const
{
    return serialPort.isOpen();
}

void QtSerialConnection::disconnect()
{
    if(serialPort.isOpen())
        serialPort.close();
}

size_t QtSerialConnection::skip(size_t maxLength)
{
    auto count = serialPort.skip(maxLength);
    if(count <= 0)
        checkState();

    return std::max(qint64(0), count);
}

size_t QtSerialConnection::peek(char* buffer, size_t maxLength)
{
    auto count = serialPort.peek(buffer, maxLength);
    if(count <= 0)
        checkState();

    return std::max(qint64(0), count);
}

size_t QtSerialConnection::read(char* buffer, size_t maxLength)
{
    auto count = serialPort.read(buffer, maxLength);
    if(count <= 0)
        checkState();

    return std::max(qint64(0), count);
}

size_t QtSerialConnection::readLine(char* buffer, size_t maxLength)
{
    auto count = serialPort.readLine(buffer, maxLength);
    if(count <= 0)
        checkState();

    return std::max(qint64(0), count);
}

void QtSerialConnection::checkState()
{
    if(serialPort.error() != QSerialPort::SerialPortError::NoError)
        hadError = true;
}

} // namespace devices

#endif
