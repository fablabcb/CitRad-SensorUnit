#ifndef GFX_RESULTVISUALIZER_HPP
#define GFX_RESULTVISUALIZER_HPP

#include "../ResultModel.hpp"
#include "../SpectrumController.hpp"
#include "ImageProvider.hpp"

#include <QImage>
#include <QMetaObject>

namespace gfx
{

class ResultVisualizer : public ImageProvider
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(SpectrumController* spectrumController READ getSpectrumController WRITE setSpectrumController NOTIFY
                   spectrumControllerChanged FINAL)
    Q_PROPERTY(ResultModel* resultModel READ getResultModel WRITE setResultModel NOTIFY resultModelChanged FINAL)
    Q_PROPERTY(float speedToBinFactor READ getSpeedToBinFactor WRITE setSpeedToBinFactor NOTIFY speedToBinFactorChanged
                   FINAL REQUIRED)
    Q_PROPERTY(int dynamicWidth READ getDynamicWidth WRITE setDynamicWidth NOTIFY dynamicWidthChanged FINAL)

  public:
    ResultVisualizer(QQuickItem* parent = nullptr);

    void addResult(SignalAnalyzer::Results const& result, size_t column);

    ResultModel* getResultModel() const { return resultModel; }
    void setResultModel(ResultModel* resultModel);

    SpectrumController* getSpectrumController() const { return spectrumController; }
    void setSpectrumController(SpectrumController* controller);

    void setSpeedToBinFactor(float newFactor);
    float getSpeedToBinFactor() const { return speedToBinFactor; }

    int getDynamicWidth() const { return dynamicWidth; }
    void setDynamicWidth(int newDynamicWidth);

  signals:
    void resultModelChanged();
    void speedToBinFactorChanged();
    void dynamicWidthChanged();
    void spectrumControllerChanged();

  private:
    void fetchLatest();

  private:
    SpectrumController* spectrumController = nullptr;
    ResultModel* resultModel = nullptr;
    QMetaObject::Connection resultConnection;
    float speedToBinFactor = 0.0f;
    size_t dynamicWidth = 0; //! how much of the image needs to be kept editable
};

} // namespace gfx

#endif
