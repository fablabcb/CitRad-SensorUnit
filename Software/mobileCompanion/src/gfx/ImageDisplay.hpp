#ifndef GFX_IMAGEDISPLAY_HPP
#define GFX_IMAGEDISPLAY_HPP

#include <QImage>
#include <QQuickPaintedItem>

namespace gfx
{

/**
 * @brief The ImageDisplay class can be used to display a QImage in QML.
 * @details The class supports a property endSeam to define where the end of the image actually is. When drawing, the
 * image will be split in two and rearranged.
 * LLLRRRRRR will be rendered as RRRRRRLLL.
 *    |seam
 * The endSeam will default to image.width and therefore the image is rendered as it is.
 */
class ImageDisplay : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariant image READ getImage WRITE setImage NOTIFY imageChanged FINAL)
    Q_PROPERTY(int endSeam READ getEndSeam WRITE setEndSeam NOTIFY endSeamChanged FINAL)

  public:
    ImageDisplay(QQuickItem* parent = nullptr);

    void paint(QPainter* painter) override;

    QVariant getImage() { return QVariant::fromValue(image); }
    void setImage(QImage* newImage);
    void setImage(QVariant newImage);

    int getEndSeam() const { return endSeam; }
    void setEndSeam(int newEnd);

    Q_INVOKABLE void forceUpdate();

  signals:
    void imageChanged();
    void endSeamChanged();

  private:
    QImage* image = nullptr;
    int endSeam = -1;
};

} // namespace gfx

#endif
