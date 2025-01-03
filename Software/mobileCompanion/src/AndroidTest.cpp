#include "AndroidTest.hpp"

#ifdef ANDROID
#include <QDebug>
#include <QtAndroidExtras/QAndroidJniEnvironment>

static char const* s_className = "de/citrad/mobileCompanion/TestClass";
static char const* s_discoverUsbSig = "(Landroid/content/Context;)I";
static char const* s_getNameSig = "(I)Ljava/lang/String;";
static char const* s_connectDeviceSig = "(Landroid/content/Context;Landroid/app/Activity;I)Z";

AndroidTest::AndroidTest()
{
    QAndroidJniEnvironment env;
    jclass javaClass = env.findClass(s_className);
    testClassObject = QAndroidJniObject(javaClass);
}

AndroidTest::~AndroidTest()
{
    close();
}

int AndroidTest::getUsbDeviceCount()
{
    auto deviceCount =
        testClassObject.callMethod<jint>("discoverUsb", s_discoverUsbSig, QtAndroid::androidContext().object());

    return deviceCount;
}

QString AndroidTest::getName(size_t deviceIndex)
{
    auto deviceName = testClassObject.callObjectMethod("getDeviceName", s_getNameSig, int(deviceIndex));

    return deviceName.toString();
}

QVariantList AndroidTest::getDeviceNames()
{
    QVariantList result;

    size_t count = getUsbDeviceCount();

    for(size_t i = 0; i < count; i++)
        result += getName(i);

    return result;
}

bool AndroidTest::connectUsb()
{
    auto ok = testClassObject.callMethod<jboolean>(
        "connectDevice",
        s_connectDeviceSig,
        QtAndroid::androidContext().object(),
        QtAndroid::androidActivity().object(),
        0);

    return ok;
}

size_t AndroidTest::read(std::vector<unsigned char>& data, size_t maxLength)
{
    auto dataObject = testClassObject.callObjectMethod("read", "(I)[B", maxLength);

    QAndroidJniEnvironment env;
    jbyteArray dataArray = dataObject.object<jbyteArray>();

    if(!dataArray)
    {
        return 0;
    }

    jsize size = env->GetArrayLength(dataArray);
    if(size == 0)
        return 0;

    jbyte* rawData = env->GetByteArrayElements(dataArray, NULL);
    for(size_t i = 0; i < size && i < data.size(); i++)
        data[i] = rawData[i];

    // image = QImage(QImage::fromData((uchar*)rawData, size, "PNG"));
    env->ReleaseByteArrayElements(dataArray, rawData, JNI_ABORT);

    return size;
}

bool AndroidTest::testJni()
{
    bool ok = true;

    return ok;
}

void AndroidTest::close()
{
    testClassObject.callMethod<void>("close");
}

/*
old stuff - for reference
bool AndroidTest::isTypeAvailable()
{
    return QAndroidJniObject::isClassAvailable(s_className);
}

bool AndroidTest::callGetTrue()
{
    return testClassObject.callMethod<jboolean>("getTrue");
}

bool AndroidTest::callGetFalse()
{
    return testClassObject.callMethod<jboolean>("getFalse");
}

int AndroidTest::callStatic(int a)
{
    // public static void notify(Context context, String message)
    jint n = a;
    return QAndroidJniObject::callStaticMethod<jint>(s_className, "myStaticFunction", "(I)I", n);
}
*/
#endif
