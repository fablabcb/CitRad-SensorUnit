#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <TimeLib.h>

#include <array>
#include <cstdint>

void setI2SFreq(int freq);
time_t getTeensy3Time();

uint32_t getTeensySerial();
const char* teensySerialNumberAsString();

#endif
