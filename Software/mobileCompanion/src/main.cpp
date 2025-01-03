#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtCore/QStandardPaths>
#include <QtQml/QQmlContext>

#include "QmlApi.hpp"

#include "gfx/ImageDisplay.hpp"
#include "gfx/ResultVisualizer.hpp"
#include "gfx/SpectrumVisualizer.hpp"
#include <shared/SignalAnalyzer.hpp>

// QtSerialPort is not supported on Android
#ifdef ANDROID
#include "AndroidTest.hpp"
#include "devices/AndroidSerialDeviceManager.hpp"
#else
#include "devices/QtSerialDeviceManager.hpp"
#endif

#include <iostream>

// use this to get qml messages to the console
void messageHandler(QtMsgType, QMessageLogContext const& logContext, QString const& msg)
{
    // silence Qt warnings from Qt internal files ...  -.-)
    if(logContext.file && QString(logContext.file).startsWith("file:///usr/lib"))
        return;

    std::string file = logContext.file == nullptr ? "<file>" : std::string(logContext.file);
    std::string fnc = logContext.function == nullptr ? "<function>" : std::string(logContext.function);

    std::cerr << file << ":" << logContext.line << " - " << fnc << ": " << msg.toStdString() << std::endl;
}

SignalAnalyzer createSignalAnalyzer(size_t sampleRate);

int main(int argc, char* argv[])
{
    // have this BEFORE the app is constructed and to have sane sizes of controls on Android
    // QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    // this is working ..
    // std::string path = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation).toStdString() + "/foo.bin";

    app.setOrganizationName("citrad");
    app.setOrganizationDomain("de.citrad");

    qInstallMessageHandler(&messageHandler);

    qmlRegisterUncreatableType<QmlApi>("citrad", 1, 0, "QmlApi", "Cannot create");
    qmlRegisterUncreatableType<DevicesController>("citrad", 1, 0, "DevicesController", "Cannot create");
    qmlRegisterUncreatableType<ResultModel>("citrad", 1, 0, "ResultModel", "Cannot create");

    qmlRegisterType<gfx::SpectrumVisualizer>("citrad", 1, 0, "SpectrumVisualizer");
    qmlRegisterType<gfx::ResultVisualizer>("citrad", 1, 0, "ResultVisualizer");
    qmlRegisterType<gfx::ImageDisplay>("citrad", 1, 0, "ImageDisplay");

    qmlRegisterSingletonType(QUrl("qrc:///Style.qml"), "citrad.style", 1, 0, "Style");

#ifdef ANDROID
    std::unique_ptr<devices::IDeviceManager> managerImpl = std::make_unique<devices::AndroidSerialDeviceManager>();
#else
    std::unique_ptr<devices::IDeviceManager> managerImpl = std::make_unique<devices::QtSerialDeviceManager>();
#endif

    size_t historySize = 4000;
#ifdef ANDROID
    historySize = 1024;
#endif
    // TODO this could be read from serial first (if we ever change the sample rate)
    size_t sampleRate = 12000;
    SpectrumController spectrumController(1024, historySize, sampleRate);
    ResultModel resultModel(historySize);

    SignalAnalyzer signalAnalyzer = createSignalAnalyzer(sampleRate);
    SignalAnalyzer::Results analyzerResults;

    DevicesController::DataReceiver receiverFunction =
        [&spectrumController,
         &signalAnalyzer,
         &analyzerResults,
         &resultModel](int type, std::vector<char> const& data, size_t timestamp) {
            switch(type)
            {
                case 0:
                    spectrumController.addSample(data, timestamp);
                    analyzerResults.spectrum = spectrumController.getLast();
                    analyzerResults.timestamp = timestamp;
                    signalAnalyzer.processData(analyzerResults);
                    resultModel.add(analyzerResults, spectrumController.getActiveSampleIdx() - 1);
                    break;
                default:
                    break;
            }
        };
#ifdef ANDROID
    DevicesController::FileDataReceiver fileReceiverFunction = {};
#else
    DevicesController::FileDataReceiver fileReceiverFunction =
        [&spectrumController, &signalAnalyzer, &analyzerResults, &resultModel](
            std::vector<float> const& data,
            size_t timestamp) {
            spectrumController.addSample(data, timestamp);
            analyzerResults.spectrum = spectrumController.getLast();
            analyzerResults.timestamp = spectrumController.getLastTimestamp();
            signalAnalyzer.processData(analyzerResults);
            resultModel.add(analyzerResults, spectrumController.getActiveSampleIdx() - 1);
        };
#endif

    LogModel logModel(10000);
    DevicesController devicesController(*managerImpl, receiverFunction, fileReceiverFunction, logModel);
    devicesController.updateDevices();
    auto devices = devicesController.getDevices();
    if(false && devices.size() > 0)
    {
        devicesController.connectTextPort(devices[0].toString());
        devicesController.connectRawPort(devices[1].toString());
    }
    else
    {
#ifndef ANDROID
        // development helper to auto-load file
        // devicesController.openRawFile(QUrl("data.bin"));
        // devicesController.readNextSamplesFromFile(1000);
#endif
    }

    QmlApi api(devicesController, logModel, spectrumController, resultModel);
#ifdef ANDROID
    api.setMobile(true);
#else
    api.setMobile(false);
#endif

    engine.load(QUrl("qrc:/Main.qml"));
    engine.rootObjects().at(0)->setProperty("api", QVariant::fromValue(&api));

    return app.exec();
}

SignalAnalyzer createSignalAnalyzer(size_t sampleRate)
{
    SignalAnalyzer result;
    SignalAnalyzer::Config config;
    config.fftWidth = 1024;
    config.signalSampleRate = sampleRate;

    result.setup(config);
    return result;
}
