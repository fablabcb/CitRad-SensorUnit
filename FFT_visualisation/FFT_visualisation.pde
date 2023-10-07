import processing.serial.*;

Serial myPort;        // The serial port
PrintWriter output;
int xStart = 70;
int xPos = xStart;         // horizontal position of the graph
float speed = 0;
float old_speed = 0;
int[] nums = new int[513];
float spec_speed = 0;
float speed_conversion = 0.9787819602; //44100/1024/44; //1.578125;
int max_frequency = 51;
String inString;

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
  // axis
  stroke(#FFFFFF);
  fill(#FFFFFF);
  //rect(0, 0, xStart-1, height);
  background(255);
  
  stroke(#ff0000);
  line(xStart-1, 0, xStart-1, height);
  for(int s =0; s < 1000; s=s+10){
    int tick_label = s;
    float tick_position = map(tick_label/speed_conversion, 1, max_frequency, height, 0);
    line(xStart-10, tick_position, xStart-1, tick_position);
    fill(#ff0000);
    textSize(20);
    textAlign(RIGHT,CENTER);
    text(tick_label, xStart-13, tick_position-5);
  }
  textAlign(CENTER, CENTER);
  translate(10, height/2);
  rotate(radians(-90));
  text("Geschwindigkeit (km/h)", 0, 0);
  
}

void draw () {
  // write to data file
  output.print(inString);
  
  // draw waterfall spectrogramm
  for(int i = 0; i < max_frequency; i++){
    //fill(xPos, 100, nums[i]);
    spec_speed = i+2.5;
    spec_speed = map(spec_speed, 1, max_frequency, 0, height);
    stroke(255-float(nums[i])*10);
    rect(xPos, height-spec_speed, 1, height/max_frequency);
  }
  
  // draw the line:
  stroke(#ff0000);
  line(xPos-1, height-old_speed, xPos, height - speed);

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
  rect(width-120, 0, 120, 80); 
  fill(#ff0000);
  textSize(104); 
  textAlign(RIGHT);
  text(round(nums[126]*speed_conversion), width, 80);
}

void keyPressed() {
  if (key == CODED) {
    if (keyCode == UP) {
      max_frequency++;
      draw_axis();
    } else if (keyCode == DOWN) {
      max_frequency--;
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
    
    nums = int(trim(split(inString, ',')));
    // trim off any whitespace:
    //println(nums[126]);
    speed = map(nums[155], 0, max_frequency, 0, height);
    //speed = map(4, 0, max_frequency, 0, height);
  }
}
