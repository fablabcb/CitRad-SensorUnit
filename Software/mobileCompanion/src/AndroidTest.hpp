#ifndef ANDROIDTEST_HPP
#define ANDROIDTEST_HPP

#ifdef ANDROID
#include <QString>
#include <QVariant>
#include <QtAndroidExtras/QtAndroid>
#include <vector>

class AndroidTest
{
  public:
    AndroidTest();
    ~AndroidTest();

  public:
    int getUsbDeviceCount();

    QString getName(size_t deviceIndex);
    QVariantList getDeviceNames();
    bool connectUsb();

    bool testJni();
    void close();

    size_t read(std::vector<unsigned char>& data, size_t maxLength);

  private:
    QAndroidJniObject testClassObject;
};

#endif
#endif // ANDROIDTEST_HPP
