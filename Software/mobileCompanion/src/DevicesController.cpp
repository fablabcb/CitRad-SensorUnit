#include "DevicesController.hpp"

#include "debug.hpp"

#include <QUrl>

#include <chrono>
#include <cmath>
#include <thread>

using namespace devices;

Worker::Worker(devices::IDeviceManager& managerImpl)
    : managerImpl(managerImpl)
    , samples(10)
{
    connect(this, &Worker::requestConnect, this, &Worker::internalConnect, Qt::QueuedConnection);
    connect(this, &Worker::requestDisconnect, this, &Worker::internalDisconnect, Qt::QueuedConnection);
}

size_t Worker::getSampleCount() const
{
    std::lock_guard<std::mutex> lock(mtx);
    return samples.getAvailableCount();
}

Worker::Sample Worker::getSample()
{
    std::lock_guard<std::mutex> lock(mtx);
    auto const& cres = samples.pop();
    return Sample{cres.type, std::move(cres.data), cres.timestamp};
}

Worker::~Worker()
{
    disconnectPort();
}

void Worker::connectPort(QString deviceId)
{
    this->deviceId = deviceId;
    emit requestConnect();
}

void Worker::disconnectPort()
{
    emit requestDisconnect();
}

void Worker::internalConnect()
{
    rawPort = managerImpl.connect(deviceId.toStdString(), IConnection::Mode::R);
    if(not rawPort)
        return;

    if(rawPort->isConnected() == false)
        internalDisconnect();
    else
    {
        timer = std::make_unique<QTimer>();
        connect(timer.get(), &QTimer::timeout, this, &Worker::readRawPort);
        timer->start(0);
        emit connectionChanged();
    }
}

void Worker::internalDisconnect()
{
    if(timer)
    {
        timer->stop();
        timer = {};
    }
    if(rawPort)
    {
        rawPort->disconnect();
        rawPort = {};
    }
    emit connectionChanged();
}

DevicesController::DevicesController(
    IDeviceManager& managerImpl,
    DataReceiver const& dataReceiver,
    FileDataReceiver const& fileDataReceiver,
    LogModel& logModel)
    : managerImpl{managerImpl}
    , logModel(logModel)
    , dataReceiver(dataReceiver)
    , fileDataReceiver(fileDataReceiver)
{
    connect(&fileTimer, &QTimer::timeout, this, &DevicesController::readNextFromFile);
    connect(&textPortTimer, &QTimer::timeout, this, &DevicesController::readTextPort);
    connect(&devicesUpdateTimer, &QTimer::timeout, this, &DevicesController::updateDevices);

    fileTimer.setSingleShot(true);
    textPortTimer.start(20);
    devicesUpdateTimer.start(2000);

    worker = std::make_unique<Worker>(managerImpl);
    worker->moveToThread(&rawReadThread);
    connect(worker.get(), &Worker::brokenPackageCounterChanged, this, &DevicesController::brokenPackageCounterChanged);
    connect(worker.get(), &Worker::connectionChanged, [this] {
        if(isRawPortConnected())
            this->logModel.add(LogModel::Type::Status, "Spectrum/Data Connection opened using " + rawPort()->getName());
        else
            this->logModel.add(LogModel::Type::Status, "Spectrum/Data Failed to create or lost connection");
        emit isRawPortConnectedChanged();
    });
    connect(worker.get(), &Worker::sampleReady, this, &DevicesController::onSampleReady, Qt::QueuedConnection);

    rawReadThread.start();
}

DevicesController::~DevicesController()
{
    if(isRawPortConnected())
        disconnectRawPort();
    if(isTextPortConnected())
        disconnectTextPort();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    rawReadThread.quit();
    rawReadThread.wait();
}

QVariantList DevicesController::getDevices() const
{
    QVariantList result;
    for(auto const& info : deviceNames)
        result += QString::fromStdString(info);

    return result;
}

bool DevicesController::isRawPortConnected() const
{
    auto const port = rawPort();
    return port && port->isConnected();
}

bool DevicesController::isTextPortConnected() const
{
    return textPort && textPort->isConnected();
}

bool DevicesController::isInFileMode() const
{
    return fileStream != nullptr;
}

bool DevicesController::isFileModeSupported() const
{
    return fileDataReceiver != nullptr;
}

void DevicesController::updateDevices()
{
    auto const newNames = managerImpl.getDeviceNames();
    if(newNames != deviceNames)
    {
        deviceNames = newNames;
        emit devicesChanged();
    }
}

void DevicesController::openRawFile(QUrl const& fileUrl)
{
    if(not isFileModeSupported())
        return;

    fileStream = std::make_unique<devices::FileInputStream>(fileUrl.path().toStdString());
    emit isInFileModeChanged();

    logModel.add(LogModel::Type::Status, "Opened file " + fileUrl.fileName().toStdString());
    logModel.add(LogModel::Type::Status, "  FFT Width:   " + std::to_string(fileStream->getFftWidth()));
    logModel.add(LogModel::Type::Status, "  Sample Rate: " + std::to_string(fileStream->getSignalSampleRate()));
}

void DevicesController::readNextSamplesFromFile(int samples)
{
    if(not isInFileMode() || samples < 0)
        return;

    samplesToReadFromFile = samples;
    readNextFromFile();
}

void DevicesController::connectRawPort(QString const& deviceId)
{
    if(isRawPortConnected())
        disconnectRawPort();

    fileStream = {};
    emit isInFileModeChanged();

    worker->connectPort(deviceId);
}
void DevicesController::connectTextPort(QString const& deviceId)
{
    if(isTextPortConnected())
        disconnectTextPort();

    textPort = managerImpl.connect(deviceId.toStdString(), IConnection::Mode::RW);
    if(textPort && textPort->isConnected())
    {
        logModel.add(LogModel::Type::Status, "TextLog Connection opened using " + textPort->getName());
        emit isTextPortConnectedChanged();
    }
    else
    {
        logModel.add(LogModel::Type::Status, "TextLog Failed to create connection");
        textPort = {};
    }
}

void DevicesController::disconnectRawPort()
{
    if(not rawPort())
        return;

    worker->disconnectPort();
    logModel.add(LogModel::Type::Status, "Spectrum/Data Connection closed");
    emit isRawPortConnectedChanged();
}
void DevicesController::disconnectTextPort()
{
    if(not textPort)
        return;

    textPort->disconnect();
    textPort = {};
    logModel.add(LogModel::Type::Status, "TextLog Connection closed");
    emit isTextPortConnectedChanged();
}

void DevicesController::sendCommand(QString const& command, QString const& text)
{
    logModel.add(LogModel::Type::Status, "<not implemented> Send " + command.toStdString() + " " + text.toStdString());
}

class DynamicBuffer
{
  public:
    DynamicBuffer(size_t size)
        : size(size)
    {
        buffer.resize(size);
    }

    void push(char data)
    {
        if(index >= size)
            return;

        buffer[index] = data;
        index++;
    }

    void push(char* data, size_t len)
    {
        if(index >= size)
            return;

        size_t sizeToCopy = std::min(len, missingCount());
        memcpy(buffer.data() + index, data, sizeToCopy);
        index += sizeToCopy;
    }

    bool isComplete() const { return index == size; }
    void reset() { index = 0; }
    void resize(size_t size)
    {
        this->size = size;
        buffer.resize(size);
        reset();
    }

    void writeTo(size_t& f, size_t start, size_t count)
    {
        f = 0;
        for(size_t i = 0; i < count; i++)
            f += static_cast<size_t>(buffer[start + i]) * std::pow(256, i);
    }

    size_t missingCount() const { return size - index; }

    std::vector<char> const& getBuffer() { return buffer; }

  private:
    std::vector<char> buffer;
    size_t index = 0;
    size_t size = 0;
};

void Worker::readRawPort()
{
    if(not rawPort)
        return;

    if(rawPort->hasError())
    {
        disconnectPort();
        return;
    }

    static size_t const bufferSize = 4096;
    static char buffer[bufferSize];

    static size_t startMarkerBytesOk = 0;
    static DynamicBuffer header(3 * 4);
    static DynamicBuffer payload(0);

    static size_t timestamp = 0;
    static size_t sampleId = 0;

    static bool isPackageBroken = false;

    while(true)
    {
        if(startMarkerBytesOk != 4)
        {
            size_t count = rawPort->peek(buffer, bufferSize);
            if(count == 0)
                return;

            for(size_t idx = 0; idx < count; idx++)
            {
                if(static_cast<unsigned char>(buffer[idx]) != 0xFF)
                {
                    startMarkerBytesOk = 0;
                    isPackageBroken = true;
                    continue;
                }
                else
                {
                    startMarkerBytesOk++;
                    if(startMarkerBytesOk == 4)
                    {
                        rawPort->skip(idx + 1);
                        break;
                    }
                }
            }

            if(startMarkerBytesOk != 4)
            {
                rawPort->skip(count);
                continue;
            }

            if(isPackageBroken)
            {
                brokenPackageCounter++;
                emit brokenPackageCounterChanged();
            }
            isPackageBroken = false;
            header.reset();
            payload.reset();
        }

        if(not header.isComplete())
        {
            size_t count = rawPort->read(buffer, header.missingCount());
            if(count == 0)
                return;

            header.push(buffer, count);
            if(header.isComplete())
            {
                size_t payloadSize = 0;
                header.writeTo(sampleId, 0, 4);
                header.writeTo(timestamp, 4, 4);
                header.writeTo(payloadSize, 8, 4);

                if(payloadSize > bufferSize)
                    throw std::runtime_error(
                        "readRawPort: Payload size exceeds buffer size by " + std::to_string(payloadSize - bufferSize));

                payload.resize(payloadSize);
            }
        }

        if(not payload.isComplete())
        {
            size_t count = rawPort->read(buffer, payload.missingCount());
            if(count == 0)
                return;

            payload.push(buffer, count);
            if(payload.isComplete())
            {
                std::lock_guard<std::mutex> lock(mtx);
                samples.push(Sample{0, payload.getBuffer(), timestamp});
                emit sampleReady();
                startMarkerBytesOk = 0;
            }
        }
    }
}

// NOTE: cuts long lines
void DevicesController::readTextPort()
{
    if(not textPort)
        return;

    if(textPort->hasError())
    {
        logModel.add(LogModel::Type::Status, "TextLog Port has encountered an error");
        disconnectTextPort();
        return;
    }

    static size_t const bufferSize = 4096;
    static char buffer[bufferSize];

    bool keepGoing = true;
    while(keepGoing)
    {
        auto len = textPort->readLine(buffer, bufferSize - 2); // has "\r\n" at end of line
        if(len < 3)
        {
            return;
        }
        if(buffer[len - 1] == '\n')
            len -= 1;
        if(buffer[len - 1] == '\r')
            len -= 1;

        logModel.add(LogModel::Type::Inbound, std::string(buffer, len));
    }
}

void DevicesController::readNextFromFile()
{
    if(not isInFileMode() || samplesToReadFromFile == 0)
        return;

    static std::vector<float> buffer;
    buffer.resize(fileStream->getBatchSize());
    size_t timestamp;
    fileStream->getNextBatch(buffer, timestamp);
    fileDataReceiver(buffer, timestamp);
    samplesToReadFromFile -= 1;
    fileTimer.start(1);
}

void DevicesController::onSampleReady()
{
    while(worker->getSampleCount() > 0)
    {
        auto const sample = worker->getSample();
        this->dataReceiver(sample.type, sample.data, sample.timestamp);
    }
}

IConnection const* DevicesController::rawPort() const
{
    return worker ? worker->rawPort.get() : nullptr;
}

IConnection* DevicesController::rawPort()
{
    return worker ? worker->rawPort.get() : nullptr;
}
