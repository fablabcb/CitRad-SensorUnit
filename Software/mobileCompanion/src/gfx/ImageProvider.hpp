#ifndef GFX_IMAGEPROVIDER_HPP
#define GFX_IMAGEPROVIDER_HPP

#include <QImage>
#include <QList>
#include <QQuickPaintedItem>

namespace gfx
{

/**
 * @brief The ImageRoll class can be used to display an image that is constantly beeing added do. Should be used with
 * ImageRoll.qml
 */
class ImageProvider : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariant image READ qml_getImage NOTIFY imageChanged FINAL)
    Q_PROPERTY(QList<QVariant> images READ getImages NOTIFY imagesChanged FINAL)
    Q_PROPERTY(int sectionsCount READ getSectionsCount WRITE setSectionsCount NOTIFY sectionsCountChanged FINAL)
    Q_PROPERTY(int sectionWidth READ getSectionWidth WRITE setSectionWidth NOTIFY sectionWidthChanged FINAL)
    Q_PROPERTY(int activeImageWidth READ getActiveImageWidth NOTIFY activeImageWidthChanged);

  public:
    ImageProvider(size_t additionalWidth, QQuickItem* parent = nullptr);

    void paint(QPainter* painter) override;

    QVariant qml_getImage() { return QVariant::fromValue(&image); }
    QList<QVariant> getImages();

    int getSectionsCount() const { return sectionsCount; }
    void setSectionsCount(int newCount);

    int getSectionWidth() const { return sectionWidth; }
    void setSectionWidth(int newWidth);
    int getActiveImageWidth() const { return activeImageWidth; }

    size_t getAdditionalWidth() const { return additionalWidth; }

  protected:
    void recreateImage();
    void saveToHistory();
    void setActiveImageWidth(size_t newWidth);

    QImage& getImage() { return image; }
    QImage const& getImage() const { return image; }

  signals:
    void imageChanged();
    void imagesChanged();
    void sectionsCountChanged();
    void sectionWidthChanged();
    void activeImageWidthChanged();

  private:
    QImage image;
    QList<QImage> images;
    size_t sectionsCount = 0;
    size_t sectionWidth = 0;
    size_t activeImageWidth = 0;
    size_t additionalWidth = 0;
};

} // namespace gfx

#endif
