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
#define SDCARD_MOSI_PIN  11   // Teensy 4 ignores this, uses pin 11
#define SDCARD_SCK_PIN   13  // Teensy 4 ignores this, uses pin 13

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
// configs:
const uint16_t sample_rate = 12000;
const uint8_t audio_input = AUDIO_INPUT_LINEIN; //AUDIO_INPUT_LINEIN or AUDIO_INPUT_MIC
const uint8_t linein_level = 15; //only relevant if AUDIO_INPUT_LINEIN is used
float mic_gain = 1.0; //only relevant if AUDIO_INPUT_MIC is used
const float max_pedestrian_speed = 10.0;
const bool iq_measurement = true;
float send_max_speed = 500;
bool write_sd = false;
const bool write_8bit = true;
const bool write_raw_data = true;
const bool write_csv_table = true;

//variables
uint16_t send_num_fft_bins; // is calculated from sen_max_speed
float max_amplitude;                   //highest signal in spectrum        
uint16_t max_freq_Index;
float mean_amplitude;
float mean_amplitude_reverse;   
int8_t direction;
float speed_conversion = (sample_rate/1024.0)/44.0;
uint16_t max_pedestrian_bin = int(max_pedestrian_speed/speed_conversion);
float pedestrian_amplitude;
uint16_t send_max_fft_bin = (uint16_t)min(512, int(send_max_speed/speed_conversion));
int input;
char command[1];
float saveDat[1024];
float peak;
uint16_t iq_offset;
uint16_t send_min_fft_bin;
File data_file;
File csv_file;
time_t timestamp;
bool send_output = false;
unsigned long time_millis;
uint16_t file_version = 1;

// IQ calibration
float alpha = 0.9; //1.10;
float psi = 0.04; //-0.04;
float A, C, D;

AudioAnalyzeFFT1024_IQ_F32   fft_IQ1024;      
AudioAnalyzePeak_F32         peak1;
AudioEffectGain_F32          I_gain;
AudioMixer4_F32              Q_mixer;

AudioInputI2S_F32            linein;           
AudioOutputI2S_F32           headphone;           
AudioControlSGTL5000         sgtl5000_1;     
AudioConnection_F32          patchCord1(linein, 0, I_gain, 0);       // I_gain I input
AudioConnection_F32          patchCord2(I_gain, 0, fft_IQ1024, 0);   // FFT I input

AudioConnection_F32          patchCord3(linein, 0, Q_mixer, 0);      // Q-mixer I input
AudioConnection_F32          patchCord3b(linein, 1, Q_mixer, 1);     // Q-mixer Q input 
AudioConnection_F32          patchCord4(Q_mixer,  0, fft_IQ1024, 1); // FFT Q input

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
  sgtl5000_1.inputSelect(audio_input); //AUDIO_INPUT_LINEIN or AUDIO_INPUT_MIC
  sgtl5000_1.micGain(mic_gain); //only relevant if AUDIO_INPUT_MIC is used
  sgtl5000_1.lineInLevel(linein_level); //only relevant if AUDIO_INPUT_LINEIN is used
  sgtl5000_1.volume(.5);
  I_gain.setGain(1);
  A = 1/alpha;
  C = -sin(psi)/(alpha*cos(psi));
  D = 1/cos(psi);
  I_gain.setGain(A);
  Q_mixer.gain(0, C);
  Q_mixer.gain(1, D);

  fft_IQ1024.windowFunction(AudioWindowHanning1024);
  fft_IQ1024.setNAverage(1);
  fft_IQ1024.setOutputType(FFT_DBFS);   // FFT_RMS or FFT_POWER or FFT_DBFS
  fft_IQ1024.setXAxis(3);

  pinMode(PIN_A3, OUTPUT); //A3=17, A8=22, A4=18
  digitalWrite(PIN_A4, LOW); 

  if(iq_measurement){
    iq_offset = 512;
    send_num_fft_bins = send_max_fft_bin * 2;
    send_max_fft_bin = send_max_fft_bin+512;
    send_min_fft_bin = (1024-send_max_fft_bin);
    
  }else{
    iq_offset = 0;
    send_min_fft_bin = 0;
    send_num_fft_bins = send_max_fft_bin;
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
    if(write_raw_data){
      if (SD.exists("RECORD.BIN")) {
        // The SD library writes new data to the end of the
        // file, so to start a new recording, the old file
        // must be deleted before new data is written.
        SD.remove("RECORD.BIN");
      }
      data_file = SD.open("RECORD.BIN", FILE_WRITE);
      data_file.write((byte*)&file_version, 2);
      data_file.write((byte*)&timestamp, 4);
      data_file.write((byte*)&send_num_fft_bins, 2);
      data_file.write((byte*)&iq_measurement, 1);
      data_file.write((byte*)&sample_rate, 2);
      data_file.flush();
    }
    if(write_csv_table){
      if(SD.exists("detections.csv")){
        SD.remove("detections.csv");
      }
      csv_file = SD.open("detections.csv", FILE_WRITE);
      csv_file.println("timestamp, speed, direction, strength, mean_amplitude, mean_amplitude_reverse, pedestrian_mean_amplitude");
      csv_file.flush();
    }
    write_sd = true;
  }

  // IOMUXC_SW_PAD_CTL_PAD_GPIO_B0_00 |= IOMUXC_PAD_SPEED(0) | IOMUXC_PAD_DSE(1) | IOMUXC_PAD_SRE(0);
  // IOMUXC_SW_PAD_CTL_PAD_GPIO_B0_01 |= IOMUXC_PAD_SPEED(0) | IOMUXC_PAD_DSE(1) | IOMUXC_PAD_SRE(0);
  // IOMUXC_SW_PAD_CTL_PAD_GPIO_B0_02 |= IOMUXC_PAD_SPEED(0) | IOMUXC_PAD_DSE(1) | IOMUXC_PAD_SRE(0);
  // IOMUXC_SW_PAD_CTL_PAD_GPIO_B0_03 |= IOMUXC_PAD_SPEED(0) | IOMUXC_PAD_DSE(1) | IOMUXC_PAD_SRE(0);
  //IOMUXC_SW_PAD_CTL_PAD_GPIO_B0_00 = 0b0000'0000'0000'0000'0001'0000'1000'1001;
}


void loop() {
  // control input from serial
  if (Serial.available() > 0) {
    input = Serial.read();

    if((input==0) & (mic_gain > 0.001)){ // - key
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

      //after https://www.faculty.ece.vt.edu/swe/argus/iqbal.pdf
      A = 1/alpha;
      C = -sin(psi)/(alpha*cos(psi));
      D = 1/cos(psi);
      I_gain.setGain(A);
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

    // detect highest frequency
    max_amplitude = -200.0;
    max_freq_Index = 0;
    mean_amplitude = 0.0;
    mean_amplitude_reverse = 0.0;
    pedestrian_amplitude = 0.0;
    direction = 1;

    //detect pedestrian
    for(i = 3+iq_offset; i < max_pedestrian_bin+iq_offset; i++){
      pedestrian_amplitude = pedestrian_amplitude + saveDat[i];
    }
    pedestrian_amplitude = pedestrian_amplitude/max_pedestrian_bin;
    
    for(i = (max_pedestrian_bin+1+iq_offset); i < send_max_fft_bin; i++) {
      mean_amplitude = mean_amplitude + saveDat[i];
      mean_amplitude_reverse = mean_amplitude_reverse + saveDat[1024-i];
      if (max(saveDat[i], saveDat[1024-i]) > max_amplitude) {
        max_amplitude = max(saveDat[i], saveDat[1024-i]);        //remember highest amplitude
        max_freq_Index = i;                    //remember frequency index
      }
    }
    if(iq_measurement){
      if(saveDat[max_freq_Index] > saveDat[1024-max_freq_Index]){
        direction = 1;
      }else{
        direction = -1;
      }
    }
        
    mean_amplitude = mean_amplitude/(send_max_fft_bin-(max_pedestrian_bin+1+iq_offset)); // TODO: is this valid when working with dB values?
    mean_amplitude_reverse = mean_amplitude_reverse/(send_max_fft_bin-(max_pedestrian_bin+1+iq_offset)); // TODO: is this valid when working with dB values?



    // save data on sd card
    if(write_sd){
      time_millis = millis();
      if(write_raw_data){
        if(write_8bit){
          data_file.write((byte*)&time_millis, 4);
          for(i = send_min_fft_bin; i < send_max_fft_bin; i++){
            data_file.write((uint8_t)-saveDat[i]);
          }
        }else{
          data_file.write((byte*)&time_millis, 4);
          for(i = send_min_fft_bin; i < send_max_fft_bin; i++){
            data_file.write((byte*)&saveDat[i], 4);
          }
        }
        data_file.flush();
      }
      if(write_csv_table){
        csv_file.print(time_millis);
        csv_file.print(", ");
        csv_file.print((max_freq_Index-iq_offset)*speed_conversion);
        csv_file.print(", ");
        csv_file.print(direction);
        csv_file.print(", ");
        csv_file.print(max_amplitude);
        csv_file.print(", ");
        csv_file.print(mean_amplitude);
        csv_file.print(", ");
        csv_file.print(mean_amplitude_reverse);
        csv_file.print(", ");
        csv_file.println(pedestrian_amplitude);
        csv_file.flush();
      }

      // SerialUSB1.print("csv sd write time: ");
      // SerialUSB1.println(millis()-time_millis);
    }

    // send data via Serial
    if(Serial && send_output){
      Serial.write((byte*)&mic_gain, 1);

      
      Serial.write((byte*)&max_freq_Index, 2);

      peak = peak1.read();
      Serial.write((byte*)&peak, 4);

      Serial.write((byte*)&(send_num_fft_bins), 2);

      // send spectrum
      for(i = send_min_fft_bin; i < send_max_fft_bin; i++)
      {
        Serial.write((byte*)&saveDat[i], 4);
      }

      send_output = false;
    }
  }
}


