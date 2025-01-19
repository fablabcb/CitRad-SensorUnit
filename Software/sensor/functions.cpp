#include "functions.h"

#include <Arduino.h>
#include <Audio.h>
#include <AudioStream_F32.h>
#include <OpenAudio_ArduinoLibrary.h>
#include <SD.h>
#include <SPI.h>
#include <SerialFlash.h>
#include <Wire.h>
#include <utility/imxrt_hw.h>

void setI2SFreq(int freq)
{
    // PLL between 27*24 = 648MHz und 54*24=1296MHz
    int n1 = 4; // SAI prescaler 4 => (n1*n2) = multiple of 4
    int n2 = 1 + (24000000 * 27) / (freq * 256 * n1);
    double C = ((double)freq * 256 * n1 * n2) / 24000000;
    int c0 = C;
    int c2 = 10000;
    int c1 = C * c2 - (c0 * c2);
    set_audioClock(c0, c1, c2, true);
    CCM_CS1CDR = (CCM_CS1CDR & ~(CCM_CS1CDR_SAI1_CLK_PRED_MASK | CCM_CS1CDR_SAI1_CLK_PODF_MASK)) |
                 CCM_CS1CDR_SAI1_CLK_PRED(n1 - 1)    // &0x07
                 | CCM_CS1CDR_SAI1_CLK_PODF(n2 - 1); // &0x3f
}

time_t getTeensy3Time()
{
    return Teensy3Clock.get();
}

#if defined ARDUINO_TEENSY40 || defined ARDUINO_TEENSY41
uint32_t getTeensySerial()
{
    uint32_t num;
    num = HW_OCOTP_MAC0 & 0xFFFFFF;
    return num;
}
#endif

const char* teensySerialNumberAsString()
{
    uint32_t sn = getTeensySerial();
    uint8_t* snp = (uint8_t*)&sn;

    static char teensySerial[12];
    sprintf(teensySerial, "%02x-%02x-%02x-%02x", snp[0], snp[1], snp[2], snp[3]);
    return teensySerial;
}
