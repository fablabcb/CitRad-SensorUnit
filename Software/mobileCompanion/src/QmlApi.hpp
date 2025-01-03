#ifndef QMLAPI_HPP
#define QMLAPI_HPP

#include "DevicesController.hpp"
#include "LogModel.hpp"
#include "ResultModel.hpp"
#include "SpectrumController.hpp"

#include <QObject>

class QmlApi : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QObject* textLog READ qml_getTextLog NOTIFY textLogChanged FINAL)
    Q_PROPERTY(DevicesController* devices READ qml_getDevices NOTIFY devicesChanged FINAL)
    Q_PROPERTY(SpectrumController* spectrum READ qml_getSpectrum NOTIFY spectrumChanged FINAL)
    Q_PROPERTY(ResultModel* results READ qml_getResultModel NOTIFY resultModelChanged FINAL)
    Q_PROPERTY(bool isMobile READ getIsMobile NOTIFY isMobileChanged FINAL)

  public:
    QmlApi(
        DevicesController& devices,
        LogModel& logModel,
        SpectrumController& spectrumController,
        ResultModel& resultModel);

    QObject* qml_getTextLog() { return &logModel; }
    DevicesController* qml_getDevices() { return &devicesController; }
    SpectrumController* qml_getSpectrum() { return &spectrumController; }
    ResultModel* qml_getResultModel() { return &resultModel; }

    bool getIsMobile() const { return isMobile; }
    void setMobile(bool newIsMobile);

  signals:
    void textLogChanged();
    void devicesChanged();
    void spectrumChanged();
    void resultModelChanged();
    void isMobileChanged();

  private:
    LogModel& logModel;
    DevicesController& devicesController;
    SpectrumController& spectrumController;
    ResultModel& resultModel;
    bool isMobile = false;
};

#endif
