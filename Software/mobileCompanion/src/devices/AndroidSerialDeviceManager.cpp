#include "AndroidSerialDeviceManager.hpp"

#ifdef ANDROID

#include <QtAndroidExtras/QAndroidJniEnvironment>

namespace devices
{

static char const* s_className = "de/citrad/companion/Serial";
static char const* s_discoverUsbSig = "(Landroid/content/Context;)I";
static char const* s_getNameSig = "(I)Ljava/lang/String;";
static char const* s_connectDeviceSig = "(Landroid/content/Context;Landroid/app/Activity;I)Z";

AndroidSerialDeviceManager::AndroidSerialDeviceManager()
{
    QAndroidJniEnvironment env;
    jclass javaClass = env.findClass(s_className);
    serialObject = QAndroidJniObject(javaClass);
}

std::vector<std::string> AndroidSerialDeviceManager::getDeviceNames()
{
    std::vector<std::string> result;

    size_t count = getUsbDeviceCount();
    for(size_t i = 0; i < count; i++)
        result.push_back(getName(i).toStdString());

    return result;
}

std::unique_ptr<IConnection> AndroidSerialDeviceManager::connect(std::string const& name, IConnection::Mode mode)
{
    auto const names = getDeviceNames();
    size_t deviceIdx = names.size();
    for(size_t i = 0; i < names.size(); ++i)
        if(names[i] == name)
            deviceIdx = i;

    if(deviceIdx == names.size())
        return {};

    return std::make_unique<AndroidSerialConnection>(deviceIdx, name, mode);
}

int AndroidSerialDeviceManager::getUsbDeviceCount()
{
    auto deviceCount =
        serialObject.callMethod<jint>("discoverUsb", s_discoverUsbSig, QtAndroid::androidContext().object());

    return deviceCount;
}

QString AndroidSerialDeviceManager::getName(size_t deviceIndex)
{
    auto deviceName = serialObject.callObjectMethod("getDeviceName", s_getNameSig, int(deviceIndex));

    return deviceName.toString();
}

AndroidSerialConnection::AndroidSerialConnection(size_t deviceId, std::string const& name, Mode mode)
    : IConnection(name, mode)
    , fifo(1)
{
    QAndroidJniEnvironment env;
    jclass javaClass = env.findClass(s_className);
    serialObject = QAndroidJniObject(javaClass);

    isOpen = serialObject.callMethod<jboolean>(
        "connectDevice",
        s_connectDeviceSig,
        QtAndroid::androidContext().object(),
        QtAndroid::androidActivity().object(),
        deviceId);

    fifo = FiFo<char>(2 * 4112);
}

bool AndroidSerialConnection::isConnected() const
{
    return isOpen;
}

void AndroidSerialConnection::disconnect()
{
    if(isOpen)
        serialObject.callMethod<void>("close");
    isOpen = false;
}

size_t AndroidSerialConnection::skip(size_t maxLength)
{
    return fifo.skip(maxLength);
}

size_t AndroidSerialConnection::getAvailableAndFetch(size_t requestedLength)
{
    size_t available = fifo.getAvailableCount();
    if(available < requestedLength)
        readNextBatch();

    return fifo.getAvailableCount();
}

size_t AndroidSerialConnection::peek(char* buffer, size_t maxLength)
{
    size_t available = getAvailableAndFetch(maxLength);
    size_t count = std::min(available, maxLength);
    return fifo.peek(buffer, count);
}

size_t AndroidSerialConnection::read(char* buffer, size_t maxLength)
{
    size_t available = getAvailableAndFetch(maxLength);
    size_t count = std::min(available, maxLength);
    if(count > 0)
        fifo.read(buffer, count);

    size_t const stillMissing = maxLength - count;
    if(stillMissing == 0)
        return count;

    available = getAvailableAndFetch(stillMissing);
    if(available == 0)
        return count;

    size_t count2 = std::min(available, stillMissing);
    fifo.read(buffer + count, count2);

    return count + count2;
}

size_t AndroidSerialConnection::readLine(char* buffer, size_t maxLength)
{
    size_t count = 0;
    while(fifo.getAvailableCount() > 0 && count < maxLength)
    {
        char c = fifo.pop();
        buffer[count] = c;
        count++;

        if(c == '\n')
            return count;
    }

    if(count == maxLength)
        return maxLength;

    readNextBatch();

    while(fifo.getAvailableCount() > 0 && count < maxLength)
    {
        char c = fifo.pop();
        buffer[count] = c;
        count++;

        if(c == '\n')
            return count;
    }

    return count;
}

void AndroidSerialConnection::readNextBatch()
{
    hadError = serialObject.callMethod<jboolean>("hasError");
    if(hadError)
        return;

    auto dataObject = serialObject.callObjectMethod("readBatch", "()[B");

    jbyteArray dataArray = dataObject.object<jbyteArray>();

    if(!dataArray)
        return;

    QAndroidJniEnvironment env;
    jsize size = env->GetArrayLength(dataArray);
    if(size == 0)
        return;

    jbyte* rawData = env->GetByteArrayElements(dataArray, NULL);
    for(size_t i = 0; i < size; i++)
        fifo.push(rawData[i]);

    env->ReleaseByteArrayElements(dataArray, rawData, JNI_ABORT);
}

} // namespace devices

#endif
