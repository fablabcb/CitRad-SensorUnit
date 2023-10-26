#include <Audio.h>
#include "OpenAudio_ArduinoLibrary.h"
#include "AudioStream_F32.h"
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <utility/imxrt_hw.h>
#include <TimeLib.h>



// Audio shield SD card:
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  7   // Teensy 4 ignores this, uses pin 11
#define SDCARD_SCK_PIN   14  // Teensy 4 ignores this, uses pin 13

// builtin SD card:
// #define SDCARD_CS_PIN    BUILTIN_SDCARD
// #define SDCARD_MOSI_PIN  11  // not actually used
// #define SDCARD_SCK_PIN   13  // not actually used


void setI2SFreq(int freq) {
  // PLL between 27*24 = 648MHz und 54*24=1296MHz
  int n1 = 4; //SAI prescaler 4 => (n1*n2) = multiple of 4
  int n2 = 1 + (24000000 * 27) / (freq * 256 * n1);
  double C = ((double)freq * 256 * n1 * n2) / 24000000;
  int c0 = C;
  int c2 = 10000;
  int c1 = C * c2 - (c0 * c2);
  set_audioClock(c0, c1, c2, true);
  CCM_CS1CDR = (CCM_CS1CDR & ~(CCM_CS1CDR_SAI1_CLK_PRED_MASK | CCM_CS1CDR_SAI1_CLK_PODF_MASK))
       | CCM_CS1CDR_SAI1_CLK_PRED(n1-1) // &0x07
       | CCM_CS1CDR_SAI1_CLK_PODF(n2-1); // &0x3f 
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  SerialUSB1.print(":");
  if(digits < 10)
    SerialUSB1.print('0');
  SerialUSB1.print(digits);
}

const uint16_t sample_rate = 22000;
uint16_t max_amplitude;                   //highest signal in spectrum              
uint16_t max_freq_Index;
float speed_conversion = (sample_rate/1024)/44.0;
int input;
char command[1];
float mic_gain = 1.0;
float saveDat[1024];
bool send_output = false;
float peak;
bool iq_measurement = true;
uint16_t send_max_fft_bins = 1024;
uint16_t send_from;
File data_file;
time_t timestamp;
unsigned long time_millis;
uint16_t file_version = 1;
bool write_sd = false;
int write_counter=0;

// IQ calibration
float alpha = 1.10;
float psi = -0.04;
float A, C, D;

AudioAnalyzeFFT1024_IQ_F32   fft_IQ1024;      
AudioAnalyzePeak_F32         peak1;
AudioEffectGain_F32          gain0;
AudioMixer4_F32              Q_mixer;



AudioInputI2S_F32            linein;           
AudioOutputI2S_F32           headphone;           
AudioControlSGTL5000         sgtl5000_1;     
AudioConnection_F32          patchCord1(linein, 0, gain0, 0);
AudioConnection_F32          patchCord2(gain0, 0, fft_IQ1024, 0); // I-channel
AudioConnection_F32          patchCord3(linein, 0, Q_mixer, 0); // I input
AudioConnection_F32          patchCord3b(linein, 1, Q_mixer, 1); // Q input
AudioConnection_F32          patchCord4(Q_mixer,  0, fft_IQ1024, 1); // Q-channel
AudioConnection_F32          patchCord5(linein, 0, peak1, 0);
AudioConnection_F32          patchCord6(linein, 0, headphone, 0);
AudioConnection_F32          patchCord7(linein, 1, headphone, 1);


void setup() {
  setI2SFreq(sample_rate);

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory_F32(400);

  // Enable the audio shield, select input, and enable output
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN); //AUDIO_INPUT_LINEIN or AUDIO_INPUT_MIC
  sgtl5000_1.micGain(0); //only relevant if AUDIO_INPUT_MIC is used
  sgtl5000_1.lineInLevel(10); //only relevant if AUDIO_INPUT_LINEIN is used
  sgtl5000_1.volume(.5);
  gain0.setGain(1);
  fft_IQ1024.windowFunction(AudioWindowHanning1024);
  fft_IQ1024.setNAverage(1);
  fft_IQ1024.setOutputType(FFT_DBFS);   // FFT_RMS or FFT_POWER or FFT_DBFS
  fft_IQ1024.setXAxis(3);

  pinMode(PIN_A4, OUTPUT); //A3=17, A8=22, A4=18
  digitalWrite(PIN_A4, LOW); 

  if(iq_measurement){
    send_from = 1024-send_max_fft_bins;
  }else{
    send_from = 0;
  }

  //============== SD card =============

  // Configure SPI
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);

  //setTime(Teensy3Clock.get());
  //timestamp = now();
  timestamp = Teensy3Clock.get();
  setTime(timestamp);

  if (!(SD.begin(SDCARD_CS_PIN))) {
    Serial.println("Unable to access the SD card");
  }else{
    if (SD.exists("RECORD.BIN")) {
      // The SD library writes new data to the end of the
      // file, so to start a new recording, the old file
      // must be deleted before new data is written.
      SD.remove("RECORD.BIN");
    }
    data_file = SD.open("RECORD.BIN", FILE_WRITE);
    data_file.write((byte*)&file_version, 2);
    data_file.write((byte*)&timestamp, 4);
    data_file.write((byte*)&send_max_fft_bins, 2);
    data_file.write((byte*)&iq_measurement, 1);
    data_file.write((byte*)&sample_rate, 2);
    data_file.flush();
    write_sd = true;
  }

  IOMUXC_SW_PAD_CTL_PAD_GPIO_B0_00 = 0b0000'0000'0000'0000'0001'0000'1011'0000;
}


void loop() {
  // control input from serial
  if (Serial.available() > 0) { // - key
    input = Serial.read();

    if((input==0) & (mic_gain > 0.001)){
      mic_gain=mic_gain-0.01;
    }
    if((input==1) & (mic_gain<10)){ // + key
      mic_gain=mic_gain+0.01;
      
    }      

    if(input==100){ // d for data
      send_output=true;
    }
    
    if(input==111){ //o
     alpha = alpha + 0.01;
    }
    if(input==108){ //l
      alpha = alpha - 0.01;
    }
    if(input==105){ //i
      psi=psi+0.01;
    }
    if(input==107){ //k
      psi=psi-0.01;
    }
    if(input==111 || input==108 || input==105 || input==107){
      SerialUSB1.print("alpha = ");
      SerialUSB1.println(alpha);
      SerialUSB1.print("psi = ");
      SerialUSB1.println(psi);
      A = 1/alpha;
      C = -sin(psi)/(alpha*cos(psi));
      D = 1/cos(psi);
      gain0.setGain(A);
      Q_mixer.gain(0, C);
      Q_mixer.gain(1, D);
    }

    unsigned long pctime;
    const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013

    if(input==84) { //T for time
      pctime = Serial.parseInt();
      if( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
        setTime(pctime); // Sync Arduino clock to the time received on the serial port
        Teensy3Clock.set(pctime); // set Teensy RTC
      }
      SerialUSB1.print("Time set to: ");
      SerialUSB1.print(year()); 
      SerialUSB1.print("-");
      SerialUSB1.print(month());
      SerialUSB1.print("-");
      SerialUSB1.print(day());
      SerialUSB1.print(" ");
      SerialUSB1.print(hour());
      printDigits(minute());
      printDigits(second());
      SerialUSB1.println(); 
    }
  }

  //generate output
  uint32_t i;

  if(fft_IQ1024.available()){
    float* pointer = fft_IQ1024.getData();
    for (int  kk=0; kk<1024; kk++) saveDat[kk]= *(pointer + kk);
    
    // save data on sd card
    if(write_sd){
      time_millis = millis();
      data_file.write((byte*)&time_millis, 4);
      data_file.write((byte*)pointer, 1024);      

      SerialUSB1.print("sd write time: ");
      SerialUSB1.println(millis()-time_millis);
      data_file.flush();
      //data_file.close();
    }

    // send data via Serial
    if(send_output){
      Serial.write((byte*)&mic_gain, 1);

      // detect highest frequency
      max_amplitude = 0;
      max_freq_Index = 0;
      for(i = 1; i < send_max_fft_bins; i++) {
        if ((saveDat[i] > 5) & (saveDat[i] > max_amplitude)) {
          max_amplitude = saveDat[i];        //remember highest amplitude
          max_freq_Index = i;                    //remember frequency index
        }
      }
      Serial.write((byte*)&max_freq_Index, 2);

      peak = peak1.read();
      Serial.write((byte*)&peak, 4);

      uint16_t number_send = send_max_fft_bins-send_from;
      Serial.write((byte*)&(number_send), 2);

      // send spectrum
      for(i = send_from; i < send_max_fft_bins; i++)
      {
        Serial.write((byte*)&saveDat[i], 4);
      }

      send_output = false;

    }
  }

}


