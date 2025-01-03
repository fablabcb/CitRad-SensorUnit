#include "debug.hpp"

#ifdef ANDROID
#include <android/log.h>
#else
#include <iostream>
#endif

void printLog(std::string const& str)
{
#ifdef ANDROID
    __android_log_write(ANDROID_LOG_DEBUG, "DC", str.c_str());
#else
    std::cout << "DC: " << str << std::endl;
#endif
}

