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
}

const int sample_rate = 44100;
uint16_t max_amplitude;                   //highest signal in spectrum              
uint16_t max_freq_Index;
float speed_conversion = (sample_rate/1024)/44.0;
int input;
int mic_gain = 16;

AudioAnalyzeFFT1024      fft1024;      
AudioAmplifier           amp1;           
AudioAnalyzePeak         peak1;          
AudioMixer4              mixer;

AudioInputI2S            mic;           
AudioOutputI2S           headphone;           
AudioConnection          patchCord1(mic, 1, mixer, 0);
AudioControlSGTL5000     sgtl5000_1;     
AudioConnection          patchCord3(mic, 0, amp1, 0);
AudioConnection          patchCord4(amp1, 0, fft1024, 0);
AudioConnection          patchCord5(mixer, 0, peak1, 0);
AudioConnection          patchCord6(mixer, 0, headphone, 0);
AudioConnection          patchCord7(mixer, 0, headphone, 1);


void setup() {
  setI2SFreq(sample_rate);

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(60);

  // Enable the audio shield, select input, and enable output
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(AUDIO_INPUT_MIC); //AUDIO_INPUT_LINEIN
  sgtl5000_1.micGain(mic_gain); 
  //sgtl5000_1.lineInLevel(0);
  sgtl5000_1.volume(.5);

  amp1.gain(1);

  fft1024.windowFunction(AudioWindowHanning1024);

  pinMode(PIN_A8, OUTPUT);
  digitalWrite(PIN_A8, LOW); 

  Serial.begin(9600);
}


void loop() {

  if (Serial.available() > 0) {
    input = Serial.read();

    if(input==0){
      mic_gain--;
    }
    if(input==1){
      mic_gain++;
    }
    mic_gain = max(0, min(63, mic_gain));
    sgtl5000_1.micGain(mic_gain);
  }

  uint32_t i;
  // uint16_t test[51];
  // for(i = 0; i <= 50; i++){
  //   if(i==10){
  //     test[i] = 40;
  //   }else{
  //     test[i] = i;
  //   }
    
  // }


  if(fft1024.available())
  {
    // output spectrum
    for(i = 0; i <= 511; i++)
    {
      Serial.print((int16_t)(fft1024.read(i) * 1000));

      
      //Serial.write((byte*)&test, sizeof(test));

      // Serial.write(lowByte(test));
      // Serial.write(highByte(test));

      Serial.print(",");
    }
                            
    // detect highest frequency
    max_amplitude = 0;
    max_freq_Index = 0;

    for(i = 0; i < 512; i++) {    
      if ((fft1024.output[i] > 5) & (fft1024.output[i] > max_amplitude)) {
        max_amplitude = fft1024.output[i];        //remember highest amplitude
        max_freq_Index = i;                    //remember frequency index
      }
    }
    Serial.print(max_freq_Index);
    Serial.print(",");


    // detect clipping / overall loudeness
    if(peak1.available()){
      Serial.print(peak1.read());
    }else{
      Serial.print("");
    }
    Serial.print(",");
    Serial.print(mic_gain);
    
    
    Serial.println("");
  }

}


