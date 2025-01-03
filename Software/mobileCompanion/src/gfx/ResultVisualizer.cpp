#include "ResultVisualizer.hpp"

#include <shared/remap.hpp>

#include <QPainter>

namespace gfx
{

ResultVisualizer::ResultVisualizer(QQuickItem* parent)
    : ImageProvider(512, parent)
{}

void ResultVisualizer::addResult(SignalAnalyzer::Results const& result, size_t column)
{
    size_t extraWidth = getAdditionalWidth();
    column = column < extraWidth ? column : extraWidth + (column - extraWidth) % this->getSectionWidth();

    {
        auto& tex = getImage();
        QPainter painter(&tex);
        QBrush brush(QColor("transparent"));
        painter.setBackground(brush);

        auto pen = painter.pen();

        auto drawPoint = [&pen, &painter](int x, int y, QColor const color) {
            pen.setColor(color);
            painter.setPen(pen);
            painter.drawPoint(x, y);
        };
        auto drawCircle = [&pen, &painter](int x, int y, QColor const color, int width) {
            pen.setColor(color);
            pen.setWidth(width);
            pen.setCapStyle(Qt::RoundCap);
            painter.setPen(pen);
            painter.drawPoint(x, y);
        };
        auto drawLine = [&pen, &painter](int x, int y1, int y2, QColor const color) {
            pen.setColor(color);
            painter.setPen(pen);
            painter.drawLine(x - 1, y1, x, y2);
        };

        static SignalAnalyzer::Results lastResult;

        float carTriggerSignalDiff = result.data.carTriggerSignal - lastResult.data.carTriggerSignal;
        static float lastCarTriggerSignalDiff = 0;

        int xOff = -result.setupData.hannWindowN / 2;
        int yBase = tex.height() / 2;

        auto remap = [yBase](auto v) { return math::remap(v).fromClamped(-128, 0).to(yBase, 0); };

        static auto const blue = QColor(0, 0, 255);
        static auto const sea = QColor(0, 255, 255, 128);
        static auto const green = QColor(0, 255, 0);
        static auto const white = QColor(255, 255, 255);
        static auto const grey = QColor(70, 70, 70, 128);

        if(column != 0)
        {
            drawLine(
                column,
                remap(lastResult.data.meanAmplitudeForCars),
                remap(result.data.meanAmplitudeForCars),
                blue);
            drawLine(
                column + xOff,
                remap(lastResult.data.carTriggerSignal),
                remap(result.data.carTriggerSignal),
                green);

            drawLine(
                column + xOff,
                yBase - 50 * lastCarTriggerSignalDiff,
                yBase - 50 * carTriggerSignalDiff,
                carTriggerSignalDiff > 0 ? blue : white);
            drawPoint(column + xOff, yBase, white.darker());
        }
        drawCircle(column, yBase - result.forward.maxAmplitudeBin, result.forward.considerAsSignal ? blue : sea, 3);
        drawCircle(column, yBase + result.reverse.maxAmplitudeBin, result.reverse.considerAsSignal ? blue : sea, 3);

        for(int i = -100; i <= 100; i += 10)
            drawPoint(column, 1.0 * yBase - i * speedToBinFactor, grey);

        lastCarTriggerSignalDiff = carTriggerSignalDiff;
        lastResult = result;
    }

    if(column + 1 == this->width())
    {
        this->saveToHistory();
        setActiveImageWidth(extraWidth);
    }
    else
        setActiveImageWidth(column + 1);

    this->update();
}

void ResultVisualizer::setResultModel(ResultModel* controller)
{
    if(resultModel)
        disconnect(resultConnection);

    resultModel = controller;
    if(resultModel)
    {
        resultConnection =
            connect(resultModel, &ResultModel::resultAdded, this, &ResultVisualizer::fetchLatest, Qt::DirectConnection);
        assert(resultConnection);
    }

    emit resultModelChanged();
}

void ResultVisualizer::setSpectrumController(SpectrumController* controller)
{
    if(spectrumController == controller)
        return;

    spectrumController = controller;
    if(spectrumController)
    {
        this->setHeight(spectrumController->getSampleSize());
        this->recreateImage();

        // divider/eraser is no longer required, but nice to know
        /*
        auto& tex = getImage();
        QPainter painter(&tex);
        auto mode = painter.compositionMode();
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.fillRect(0, 0, tex.width(), tex.height(), Qt::transparent);
        painter.setCompositionMode(mode);
        */
    }

    emit spectrumControllerChanged();
}

void ResultVisualizer::setSpeedToBinFactor(float newFactor)
{
    if(newFactor == speedToBinFactor)
        return;

    speedToBinFactor = newFactor;
    emit speedToBinFactorChanged();
}

void ResultVisualizer::setDynamicWidth(int newDynamicWidth)
{
    if(dynamicWidth == static_cast<size_t>(newDynamicWidth))
        return;

    dynamicWidth = newDynamicWidth;
    emit dynamicWidthChanged();
}

void ResultVisualizer::fetchLatest()
{
    addResult(resultModel->getLastCompleteResult(), resultModel->getLastSampleIndex());
}

} // namespace gfx
