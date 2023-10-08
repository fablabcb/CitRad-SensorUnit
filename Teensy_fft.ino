/*
 * A simple hardware test which receives audio from the audio shield
 * Line-In pins and send it to the Line-Out pins and headphone jack.
 *
 * This example code is in the public domain.
 */

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <utility/imxrt_hw.h>
  
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
//Serial.printf("SetI2SFreq(%d)\n",freq);
}

AudioAnalyzeFFT1024      fft1024_1;      //xy=862.3333129882812,439.33331298828125
AudioAmplifier           amp1;           //xy=425,443
AudioAnalyzePeak         peak1;          //xy=507,307
AudioFilterFIR           fir1; 
AudioAnalyzeNoteFrequency notefreq1;      
AudioMixer4               mixer;


AudioInputI2S            mic;           //xy=200,69
AudioOutputI2S           headphone;           //xy=365,94
AudioConnection          patchCord1(mic, 0, mixer, 0);
//AudioConnection          patchCord2(mic, 1, mixer, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=302,184
AudioConnection          patchCord3(mic, amp1);
AudioConnection          patchCord4(amp1, 0, fft1024_1, 0);
AudioConnection          patchCord5(mixer, 0, peak1, 0);
AudioConnection          patchCord6(mixer, 0, headphone, 0);
AudioConnection          patchCord7(mixer, 0, headphone, 1);
AudioConnection          patchCord8(mixer, 0, notefreq1, 0);
// GUItool: end automatically generated code

#define TAP_NUM   36
short filt_coeff[TAP_NUM] = {
  28,
  -2,
  -108,
  -287,
  -434,
  -387,
  -53,
  449,
  783,
  578,
  -250,
  -1290,
  -1727,
  -791,
  1697,
  5046,
  7932,
  9070,
  7932,
  5046,
  1697,
  -791,
  -1727,
  -1290,
  -250,
  578,
  783,
  449,
  -53,
  -387,
  -434,
  -287,
  -108,
  -2,
  28,
  0
};


void setup() {
  setI2SFreq(11000);

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(60);

  // Enable the audio shield, select input, and enable output
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(AUDIO_INPUT_MIC);
  sgtl5000_1.micGain(8); //16 is good
  //sgtl5000_1.lineInLevel(0);
  sgtl5000_1.volume(.5);

  //fir1.begin(filt_coeff, TAP_NUM);
  notefreq1.begin(1);

  amp1.gain(1);

  fft1024_1.windowFunction(AudioWindowHanning1024);

  pinMode(PIN_A8, OUTPUT);
  digitalWrite(PIN_A8, LOW); 

  Serial.begin(1000000);
}


void loop() {

  uint32_t i;
  // uint16_t test[51];
  // for(i = 0; i <= 50; i++){
  //   if(i==10){
  //     test[i] = 40;
  //   }else{
  //     test[i] = i;
  //   }
    
  // }


  if(fft1024_1.available())
  {

    

    // max speed fft bin: 154*(44100/1024)/44 = 150,732 km/h
    for(i = 0; i <= 511; i++)
    {
      Serial.print((int16_t)(fft1024_1.read(i) * 1000));

      
      //Serial.write((byte*)&test, sizeof(test));

      // Serial.write(lowByte(test));
      // Serial.write(highByte(test));

      Serial.print(",");
    }
    
    if(notefreq1.available()){
      float speed = notefreq1.read()/44;
      //float prob = notefreq1.probability();
      //if(speed>5.0){
        //Serial.printf("speed: %3.2f | Probability: %.2f\n", speed, prob);
        Serial.print(speed);
      //}
      
    }else{
      Serial.print("0");
    }
    Serial.print(",");

    if(peak1.available()){
      Serial.print(peak1.read());
    }else{
      Serial.print("");
    }
    
    Serial.println("");
  }

}


