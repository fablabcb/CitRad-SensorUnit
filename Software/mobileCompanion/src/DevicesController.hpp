#ifndef DEVICESCONTROLLER_HPP
#define DEVICESCONTROLLER_HPP

#include "LogModel.hpp"
#include "devices/IDeviceManager.hpp"
#include "devices/InputStream.hpp"
#include "shared/FiFo.hpp"

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QVariant>

#include <functional>
#include <memory>
#include <mutex>
#include <string>

class Worker : public QObject
{
    Q_OBJECT

  public:
    struct Sample
    {
        int type;
        std::vector<char> data;
        size_t timestamp;
    };
    Worker(devices::IDeviceManager& managerImpl);
    ~Worker();

    void connectPort(QString deviceId);
    void disconnectPort();

    std::unique_ptr<devices::IConnection> rawPort;
    size_t brokenPackageCounter = 0;

    size_t getSampleCount() const;
    Sample getSample();

  signals:
    void sampleReady();
    void requestConnect();
    void requestDisconnect();
    void brokenPackageCounterChanged();
    void connectionChanged();

  private:
    void readRawPort();
    void internalConnect();
    void internalDisconnect();

  private:
    devices::IDeviceManager& managerImpl;
    std::unique_ptr<QTimer> timer;
    QString deviceId;

    mutable std::mutex mtx;
    FiFo<Sample> samples;
};

class DevicesController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariant allDevices READ getDevices NOTIFY devicesChanged FINAL)
    Q_PROPERTY(bool isRawPortConnected READ isRawPortConnected NOTIFY isRawPortConnectedChanged FINAL)
    Q_PROPERTY(bool isTextPortConnected READ isTextPortConnected NOTIFY isTextPortConnectedChanged FINAL)
    Q_PROPERTY(bool isInFileMode READ isInFileMode NOTIFY isInFileModeChanged FINAL)
    Q_PROPERTY(bool isFileModeSupported READ isFileModeSupported NOTIFY isFileModeSupportedChanged FINAL)
    Q_PROPERTY(int brokenPackageCounter READ getBrokenPackageCounter NOTIFY brokenPackageCounterChanged FINAL)

  public:
    Q_INVOKABLE void updateDevices();

    Q_INVOKABLE void openRawFile(QUrl const& fileUrl);
    Q_INVOKABLE void readNextSamplesFromFile(int samples);

    Q_INVOKABLE void connectRawPort(QString const& deviceId);
    Q_INVOKABLE void connectTextPort(QString const& deviceId);
    Q_INVOKABLE void disconnectRawPort();
    Q_INVOKABLE void disconnectTextPort();

    Q_INVOKABLE void sendCommand(QString const& command, QString const& text);

  public:
    using DataReceiver = std::function<void(int, std::vector<char> const&, size_t timestamp)>;
    using FileDataReceiver = std::function<void(std::vector<float> const&, size_t timestamp)>;

  public:
    DevicesController(
        devices::IDeviceManager& managerImpl,
        DataReceiver const& dataReceiverFunction,
        FileDataReceiver const& fileDataReceiver,
        LogModel& logModel);
    ~DevicesController();

    QVariantList getDevices() const;
    bool isRawPortConnected() const;
    bool isTextPortConnected() const;
    bool isInFileMode() const;
    bool isFileModeSupported() const;
    int getBrokenPackageCounter() const { return worker ? worker->brokenPackageCounter : 0; }

  signals:
    void devicesChanged();
    void isRawPortConnectedChanged();
    void isTextPortConnectedChanged();
    void isInFileModeChanged();
    void isFileModeSupportedChanged();
    void brokenPackageCounterChanged();

  private:
    void toggleSlot(
        std::unique_ptr<devices::IConnection>& slot,
        std::string const& deviceId,
        devices::IConnection::Mode mode,
        std::string const& logPrefix);

    void readRawPort();
    void readTextPort();
    void readNextFromFile();
    devices::IConnection const* rawPort() const;
    devices::IConnection* rawPort();
    void onSampleReady();

  private:
    devices::IDeviceManager& managerImpl;
    std::unique_ptr<devices::FileInputStream> fileStream;

    LogModel& logModel;

    std::unique_ptr<devices::IConnection> textPort;

    QTimer fileTimer;
    QTimer textPortTimer;
    QTimer devicesUpdateTimer;

    DataReceiver dataReceiver;
    FileDataReceiver fileDataReceiver;
    QObject* spectrumVisualizer = nullptr;
    std::vector<std::string> deviceNames;

    size_t samplesToReadFromFile = 0;

  private:
    QThread rawReadThread;
    std::unique_ptr<Worker> worker;
};

#endif
