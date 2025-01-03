#include "ImageProvider.hpp"

#include <QPainter>

#include <iostream>

namespace gfx
{

ImageProvider::ImageProvider(size_t additionalWidth, QQuickItem* parent)
    : QQuickPaintedItem(parent)
    , additionalWidth(additionalWidth)
{}

void ImageProvider::paint(QPainter* painter)
{
    painter->drawImage(QRect(0, 0, this->width(), this->height()), image, QRect(0, 0, this->width(), this->height()));
}

QList<QVariant> ImageProvider::getImages()
{
    QList<QVariant> result;
    for(auto const& i : images)
        result.push_back(QVariant::fromValue(&i));
    return result;
}

void ImageProvider::setSectionWidth(int newWidth)
{
    if(newWidth == static_cast<int>(sectionWidth))
        return;

    images.clear();
    sectionWidth = newWidth;
    this->setWidth(newWidth + additionalWidth);

    emit imagesChanged();
    emit sectionWidthChanged();
}

void ImageProvider::setSectionsCount(int newSize)
{
    if(sectionsCount == static_cast<size_t>(newSize) || newSize < 1)
        return;

    bool removedSome = false;
    while(newSize < images.size())
    {
        images.pop_front();
        removedSome = true;
    }

    sectionsCount = newSize;
    emit sectionsCountChanged();

    if(removedSome)
        emit imagesChanged();
}

void ImageProvider::recreateImage()
{
    image = QImage{static_cast<int>(this->width()), static_cast<int>(this->height()), QImage::Format::Format_RGBA8888};
    image.fill(Qt::transparent);
    this->update();
    emit imageChanged();
}

void ImageProvider::saveToHistory()
{
    if(additionalWidth == 0)
    {
        images.push_back(image);
        recreateImage();
    }
    else
    {
        QImage start = image.copy(0, 0, sectionWidth, image.height());
        images.push_back(start);

        image = image.copy(sectionWidth, 0, image.width(), image.height());
    }

    // the provider itself counts as one image, but it is incomplete!
    if(images.size() > static_cast<int>(sectionsCount))
        images.pop_front();

    emit imagesChanged();
}

void ImageProvider::setActiveImageWidth(size_t newWidth)
{
    activeImageWidth = newWidth;
    emit activeImageWidthChanged();
}

} // namespace gfx
