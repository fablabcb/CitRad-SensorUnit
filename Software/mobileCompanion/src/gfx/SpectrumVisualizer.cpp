#include "SpectrumVisualizer.hpp"

#include "SpectrumColorizer.hpp"

#include <shared/remap.hpp>

#include <QPainter>

#include <iostream>

namespace gfx
{

SpectrumVisualizer::SpectrumVisualizer(QQuickItem* parent)
    : ImageProvider(0, parent)
{
    gfx::SpectrumColorizer colorizer;
    colorizer.generateColorsIfRequried(128);
    for(auto const& rawColor : colorizer.getColorPalette())
        colorPalette.append(QColor(255.0 * rawColor[0], 255.0 * rawColor[1], 255.0 * rawColor[2]));

    connect(this, &ImageProvider::sectionWidthChanged, [this] {
        overviewImage = QImage(this->getSectionWidth() * this->getSectionsCount(), 64, QImage::Format::Format_RGB16);
        overviewImage.fill("black");
        emit overviewImageChanged();
    });
    connect(this, &ImageProvider::sectionsCountChanged, [this] {
        overviewImage = QImage(this->getSectionWidth() * this->getSectionsCount(), 64, QImage::Format::Format_RGB16);
        overviewImage.fill("black");
        emit overviewImageChanged();
    });
}

void SpectrumVisualizer::addSpectrum(
    std::vector<float> const& data, size_t column, float lowerValueRange, float upperValueRange)
{
    overviewImageSeam = column % (getSectionWidth() * getSectionsCount());
    column = column % static_cast<size_t>(this->width());

    {
        auto& tex = getImage();
        assert(static_cast<int>(data.size()) == tex.height());
        QPainter painter(&tex);

        auto pen = painter.pen();
        size_t x = column;
        for(size_t row = 0; row < data.size(); row++)
        {
            size_t colorIndex =
                math::remap(data[row]).fromClamped(lowerValueRange, upperValueRange).to(0, colorPalette.size() - 1);
            size_t y = data.size() - row;
            pen.setColor(colorPalette[colorIndex]);
            painter.setPen(pen);
            painter.drawPoint(x, y);
        }

        QPainter overviewPainter(&overviewImage);
        QRect target(overviewImageSeam, 0, 1, overviewImage.height());
        QRect source(column, 0, 1, tex.height());
        overviewPainter.drawImage(target, tex, source);
        emit overviewImageSeamChanged();
    }

    if(column + 1 == this->width())
    {
        this->saveToHistory();
        setActiveImageWidth(0);
    }
    else
        setActiveImageWidth(column + 1);

    this->update();
    emit textureUpdated();
}

void SpectrumVisualizer::setColorPalette(QList<QColor> const& newPalette)
{
    if(newPalette == colorPalette)
        return;

    colorPalette = newPalette;
    emit colorPaletteChanged();
}

void SpectrumVisualizer::setSpectrumController(SpectrumController* controller)
{
    if(spectrumController == controller)
        return;

    if(spectrumController)
        disconnect(spectrumConnection);

    spectrumController = controller;
    if(spectrumController)
    {
        spectrumConnection =
            connect(spectrumController, &SpectrumController::sampleAdded, this, &SpectrumVisualizer::fetchLatest);

        this->setHeight(spectrumController->getSampleSize());
        this->recreateImage();
    }

    emit spectrumControllerChanged();
}

void SpectrumVisualizer::fetchLatest()
{
    addSpectrum(
        spectrumController->getLast(),
        spectrumController->getActiveSampleIdx(),
        spectrumController->getLowerValueRange(),
        spectrumController->getUpperValueRange());
}

} // namespace gfx
