import processing.serial.*;

Serial myPort;        // The serial port
PrintWriter output;
int xStart = 70;
int xPos = xStart;         // horizontal position of the graph
float speed = 0;
float old_speed = 0;
int num_fft_bins = 511;
float[] nums = new float[num_fft_bins+2];
float spec_speed = 0;
int sample_rate = 44100;
float fft_bin_width = sample_rate/1024;
float speed_conversion = fft_bin_width/44.0; //44100/1024/44; //1.578125;
int max_frequency = 100;

String inString;
float peak;
float oldpeak = 0;
int baseline;
 

void setup () {
  // set the window size:
  fullScreen();
  //size(1000, 500);

  // List all the available serial ports
  // if using Processing 2.1 or later, use Serial.printArray()
  println(Serial.list());

  // I know that the first port in the serial list on my Mac is always my
  // Arduino, so I open Serial.list()[0].
  // Open whatever port is the one you're using.
  myPort = new Serial(this, "/dev/tty.usbmodem142945601", 9600);

  // don't generate a serialEvent() unless you get a newline character:
  myPort.bufferUntil('\n');

  //create file to write to
  output = createWriter( "recorded_data/radar_data_" + timestamp() + ".csv" );

  // set initial background:
  background(255);
  
  draw_axis();
  

  
}

void draw_axis () { 
  baseline = height-50;
  // axis
  stroke(#FFFFFF);
  fill(#FFFFFF);
  //rect(0, 0, xStart-1, height);
  background(255);
  
  stroke(#ff0000);
  line(xStart-1, 0, xStart-1, baseline);
  for(int s =0; s < 1000; s=s+5){
    int tick_label = s;
    float tick_position = map(tick_label/speed_conversion, 0, max_frequency, baseline, 0);
    line(xStart-10, tick_position, xStart-1, tick_position);
    fill(#ff0000);
    textSize(20);
    textAlign(RIGHT,CENTER);
    text(tick_label, xStart-13, tick_position-5);
  }
  textAlign(CENTER, CENTER);
  translate(10, baseline/2);
  rotate(radians(-90));
  text("Geschwindigkeit (km/h)", 0, 0);
  
}

void draw () {
  // write to data file
  output.print(inString);
  
  // draw waterfall spectrogramm
  for(int i = 0; i < max_frequency; i++){
    //fill(xPos, 100, nums[i]);
    spec_speed = i;
    spec_speed = map(spec_speed, 0, max_frequency, 0, baseline);
    stroke(255-nums[i]*10);
    rect(xPos, baseline-spec_speed, 1, -baseline/max_frequency);
  }
  
  // draw the line:
  stroke(#ff0000);
  //if(peak>0.05){
    line(xPos-1, map(old_speed/speed_conversion, 0, max_frequency, baseline, 0), xPos, map(speed/speed_conversion, 0, max_frequency, baseline, 0));
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
  rect(width-220, 0, 220, 90); 
  fill(#ff0000);
  textSize(104); 
  textAlign(RIGHT);
  if(peak > 0.01){
    text(round(speed), width, 80);
  }
  if(peak >= 1.0){
    fill(#ff0000);
  }else{
    fill(#51A351);
  }
  rect(width/2-50, 0, 50, 30);
}

void keyPressed() {
  if (key == CODED) {
    if (keyCode == UP) {
      max_frequency = min(num_fft_bins, max_frequency+1);
      draw_axis();
    } else if (keyCode == DOWN) {
      max_frequency = max(40, max_frequency-1);
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

void serialEvent (Serial myPort) {
  // get the ASCII string:
  inString = myPort.readStringUntil('\n');
  

  if (inString != null) {
    old_speed = speed;
    nums = float(trim(split(inString, ',')));
    speed = nums[num_fft_bins+1];
    peak = nums[num_fft_bins+2];
    oldpeak = max(peak, oldpeak);
    println(round(oldpeak*100)/100.0);
    oldpeak = oldpeak*0.999;
  }
}
