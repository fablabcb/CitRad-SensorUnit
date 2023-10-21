import processing.serial.*;

Serial myPort;        // The serial port
PrintWriter output;
int xStart = 70;
int yStart = 50;
int xPos = xStart;         // horizontal position of the graph
int speed = 0;
int oldspeed = 0;
int num_fft_bins;
float spec_speed = 0;
int sample_rate = 44100;
float fft_bin_width = sample_rate/1024;
float speed_conversion = fft_bin_width/44.0; //44100/1024/44; //1.578125;
float max_frequency;

String inString;
float oldpeak = 0;
int baseline;
PImage img;
int zoom = 0;
int step;

int[] num = new int[512];
byte[] inBuffer = new byte[16];
int mic_gain;
int max_freq_Index;
int peak_int;
float peak;
float min_fft = 0;
float max_fft = 0;
int start_sequence;
boolean axis_drawn = false;
boolean iq_graph = true;
float axis_start;

float log10 (float x) {
  return (log(x) / log(10));
}


void setup () {
  size(1400, 561);
  // set the window size:
  //size(displayWidth, displayHeight);
  //fullScreen();
  
  windowResizable(true);
  
  // List all the available serial ports
  // if using Processing 2.1 or later, use Serial.printArray()
  println(Serial.list());

  // I know that the first port in the serial list on my Mac is always my
  // Arduino, so I open Serial.list()[0].
  // Open whatever port is the one you're using.
  myPort = new Serial(this, Serial.list()[9], 230400);

  // don't generate a serialEvent() unless you get a newline character:
  myPort.bufferUntil('\n');


  //create file to write to
  output = createWriter( "recorded_data/radar_data_" + timestamp() + ".csv" );

  // set initial background:
  background(255);
}

void windowResized() {
  xPos = xStart;
  axis_drawn= false;
}

void draw_axis () { 
  baseline = height-yStart;
  img = createImage(1, baseline, RGB);
  
  
  
  
  // axis
  stroke(#FFFFFF);
  fill(#FFFFFF);
  background(255);
  
  stroke(#ff0000);
  line(xStart-1, 0, xStart-1, baseline);
  
  //get nice tick spacing
  float max_frequency = (num_fft_bins*speed_conversion-zoom);
  float tick;
  float minimum = max_frequency / 20;
  float magnitude = pow(10, floor(log10(minimum)));
  float residual = minimum / magnitude;
  if(residual > 5){
    tick = 10 * magnitude;
  }else{
    if(residual > 2){
       tick = 5 * magnitude;
    }else{
      if(residual > 1){
        tick = 2 * magnitude;
      }else{
        tick = magnitude;
      }
    }
  }
  step = int(tick);
  /////////////////
  
  if(iq_graph){
    axis_start = -max_frequency;
  }else{
    axis_start = 0;
  }
     
  for(int s = 0; s < max_frequency; s=s+step){
    int tick_label = s;
    float tick_position = map(tick_label, axis_start, max_frequency, baseline, 0);
    line(xStart-10, tick_position, xStart-1, tick_position);
    fill(#ff0000);
    textSize(20);
    textAlign(RIGHT,CENTER);
    text(tick_label, xStart-13, tick_position-5);
  }
  
  if(iq_graph){
    for(int s = 0; s > -max_frequency; s=s-step){
      int tick_label = s;
      float tick_position = map(tick_label, axis_start, max_frequency, baseline, 0);
      line(xStart-10, tick_position, xStart-1, tick_position);
      fill(#ff0000);
      textSize(20);
      textAlign(RIGHT,CENTER);
      text(tick_label, xStart-13, tick_position-5);
    }
  }
  
  textAlign(CENTER, CENTER);
  translate(10, baseline/2);
  rotate(radians(-90));
  text("Geschwindigkeit (km/h)", 0, 0);
  
  rotate(radians(90));
  translate(-10,-baseline/2);

}

void draw () {
  
  //================== read data from serial =============================
  
  oldspeed = speed;
  myPort.clear();
  myPort.write("d");
  
  while(myPort.available()<9){
    delay(1);
  }

  //print("mic_gain:");
  mic_gain = (myPort.read());
  //print(mic_gain);
  
  //print(", max_freq_index:");
  max_freq_Index = (myPort.read()) + (myPort.read()<<8);
  speed = max_freq_Index;
  //print(max_freq_Index);
  
  //print(", peak:");
  peak_int = myPort.read() + (myPort.read()<<8) + (myPort.read()<<16) + (myPort.read()<<24);
  peak = Float.intBitsToFloat(peak_int);
  //print(round(peak*100)/100.0);
  
  //print(", numfft:");
  num_fft_bins = (myPort.read()) + (myPort.read()<<8);
  //print(num_fft_bins);
  
  int[] nums_int = new int[num_fft_bins];
  float[] nums = new float[num_fft_bins];

  for(int i = 0; i < num_fft_bins; i++){
      //print(",");
      while(myPort.available()<4){
        delay(1);
      }
      nums_int[i] = myPort.read() + (myPort.read()<<8) + (myPort.read()<<16) + (myPort.read()<<24);
      nums[i] = Float.intBitsToFloat(nums_int[i]);
      min_fft = min(min_fft, nums[i]);
      max_fft = max(max_fft, nums[i]);
      //print(nums[i]);
  }
  
  //print(",min:");
  //print(min_fft);
  //print(",max:");
  //print(max_fft);
  
  //println();
  
  oldpeak = oldpeak*0.999;
  
  // write to data file
  output.print(inString);
  
  
  
  //=========== draw waterfall spectrogramm =======================
  
  if(!axis_drawn){
    draw_axis();
    axis_drawn = true;
  }
  
  img.loadPixels();
  int j = img.height-1;
  int i;
  int pixel;
  
  for(pixel=0; pixel < img.height; pixel++){
    i = int(map(pixel, 0, img.height-1, num_fft_bins-1-zoom, 0));
    float col = map(nums[i], -190, 1, 255, 0);
    img.pixels[pixel] = color(col,col,col);
  }
  
  img.updatePixels();
  image(img, xPos,0);
  
  // draw the line:
  stroke(#ff0000);
  //if(peak>0.05){
    //line(xPos-1, map(oldspeed, 0, (num_fft_bins/zoom), baseline, 0), xPos, map(speed, 0, num_fft_bins/zoom, baseline, 0));
  //}

  // at the edge of the screen, go back to the beginning:
  if (xPos >= width) {
    xPos = xStart;
    //background(0);
  } else {
    // increment the horizontal position:
    xPos++;
  }
  
  //draw speed as text
  fill(255);
  stroke(255);
  //rect(width-220, 0, 220, 90); 
  fill(#ff0000);
  textSize(104); 
  textAlign(RIGHT);
  //if(peak > 0.01){
    //text(round(speed*speed_conversion), width, 80);
  //}
  if(peak >= 1.0){
    fill(#ff0000);
  }else{
    fill(#51A351);
  }
  rect(width/2-25, 0, 50, 30);
  fill(#000000);
  textAlign(CENTER, TOP);
  textSize(23);
  text(round(mic_gain), width/2, 0);
}

void keyPressed() {
  if (key == CODED) {
    if (keyCode == UP) {
      zoom = min(zoom+10,480);
      xPos = xStart;
      draw_axis();
    } else if (keyCode == DOWN) {
      zoom = max(zoom-10, 0);
      xPos = xStart;
      draw_axis();
    }

  } else if (key == 's') {
    String filename = "screenshots/radar_spectrum_" + timestamp() + ".png"; 
    println(filename);
    save(filename);
  } else if (key== 'q') {
    output.flush();  // Writes the remaining data to the file
    output.close();  // Finishes the file
    exit();  // Stops the program
  } else {
   myPort.write(key);
   print("sending key");
   println(key);
  }
  
}

String timestamp() {
  String[] timestamp = new String[6];
  timestamp[0] = String.valueOf(year());
  timestamp[1] = String.valueOf(month());
  timestamp[2] = String.valueOf(day());
  timestamp[3] = String.valueOf(hour());
  timestamp[4] = String.valueOf(minute());
  timestamp[5] = String.valueOf(second());
  String timestamp_str = join(timestamp, "-");
  return(timestamp_str);
}
