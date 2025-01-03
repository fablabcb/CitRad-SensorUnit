#ifndef SPECTRUMVISUALIZER_HPP
#define SPECTRUMVISUALIZER_HPP

#include "../SpectrumController.hpp"
#include "ImageProvider.hpp"

#include <QColor>
#include <QImage>
#include <QMetaObject>

#include <vector>

namespace gfx
{

class SpectrumVisualizer : public ImageProvider
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(SpectrumController* spectrumController READ getSpectrumController WRITE setSpectrumController NOTIFY
                   spectrumControllerChanged FINAL)
    Q_PROPERTY(QList<QColor> colorPalette READ getColorPalette NOTIFY colorPaletteChanged FINAL)
    Q_PROPERTY(QVariant overviewImage READ qml_getOverviewImage NOTIFY overviewImageChanged FINAL)
    Q_PROPERTY(int overviewImageSeam READ getOverviewImageSeam NOTIFY overviewImageSeamChanged FINAL)

  public:
    SpectrumVisualizer(QQuickItem* parent = nullptr);

    void addSpectrum(std::vector<float> const& data, size_t column, float lowerValueRange, float upperValueRange);

    QList<QColor> getColorPalette() const { return colorPalette; }
    void setColorPalette(QList<QColor> const& newPalette);

    SpectrumController* getSpectrumController() const { return spectrumController; }
    void setSpectrumController(SpectrumController* controller);

    QVariant qml_getOverviewImage() const { return QVariant::fromValue(&overviewImage); }
    int getOverviewImageSeam() const { return overviewImageSeam; }

  signals:
    void colorPaletteChanged();
    void textureUpdated();
    void spectrumControllerChanged();
    void overviewImageChanged();
    void overviewImageSeamChanged();

  private:
    void fetchLatest();

  private:
    QImage overviewImage;
    SpectrumController* spectrumController = nullptr;
    QList<QColor> colorPalette;
    QMetaObject::Connection spectrumConnection;
    size_t overviewImageSeam = 0;
};

} // namespace gfx

#endif // SPECTRUMVISUALIZER_HPP
