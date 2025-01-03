#include "ImageDisplay.hpp"

#include <QPainter>

namespace gfx
{

ImageDisplay::ImageDisplay(QQuickItem* parent)
    : QQuickPaintedItem(parent)
{}

void ImageDisplay::paint(QPainter* painter)
{
    if(!image)
        return;

    auto const& tex = *image;

    if(endSeam >= tex.width() || endSeam == -1)
    {
        painter->drawImage(QRect(0, 0, this->width(), this->height()), tex);
    }
    else
    {
        int const newW = endSeam + 1;
        int const oldW = tex.width() - newW;

        QRect const sourceA(0, 0, newW, tex.height());
        QRect const sourceB(newW, 0, oldW, tex.height());

        QRect targetA(oldW, 0, newW, this->height());
        QRect targetB(0, 0, oldW, this->height());

        if(this->width() != tex.width())
        {
            int const targetNewW = 1.0 * (endSeam + 1) / tex.width() * this->width();
            int const targetOldW = this->width() - targetNewW;

            targetA = QRect(targetOldW, 0, targetNewW, this->height());
            targetB = QRect(0, 0, targetOldW, this->height());
        }

        painter->drawImage(targetA, tex, sourceA);
        painter->drawImage(targetB, tex, sourceB);
    }
}

void ImageDisplay::setImage(QVariant newImage)
{
    assert(newImage.canConvert<QImage*>());

    QImage* img = newImage.value<QImage*>();
    setImage(img);
}

void ImageDisplay::setEndSeam(int newEndSeam)
{
    if(newEndSeam == static_cast<int>(endSeam))
        return;

    endSeam = newEndSeam;
    emit endSeamChanged();
    this->update();
}

void ImageDisplay::forceUpdate()
{
    this->update();
}

void ImageDisplay::setImage(QImage* newImage)
{
    if(image == newImage)
        return;

    image = newImage;
    emit imageChanged();

    this->update();
}

} // namespace gfx
