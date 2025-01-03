#include "QmlApi.hpp"

QmlApi::QmlApi(
    DevicesController& devicesController,
    LogModel& logModel,
    SpectrumController& spectrumController,
    ResultModel& resultModel)
    : logModel(logModel)
    , devicesController(devicesController)
    , spectrumController(spectrumController)
    , resultModel(resultModel)
{}

void QmlApi::setMobile(bool newIsMobile)
{
    if(newIsMobile == isMobile)
        return;

    isMobile = newIsMobile;
    emit isMobileChanged();
}
