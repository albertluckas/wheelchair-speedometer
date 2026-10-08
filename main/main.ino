//MADE wheelchair code, based on the demos for the lcd screen
//This program is a demo of displaying string

//when using the BREAKOUT BOARD only and using these hardware spi lines to the LCD,
//the SDA pin and SCK pin is defined by the system and can't be modified.
//if you don't need to control the LED pin,you can set it to 3.3V and set the pin definition to -1.
//other pins can be defined by youself,for example
//pin usage as follow:
//             CS  DC/RS  RESET  SDI/MOSI  SDO/MISO  SCK  LED    VCC     GND
//Arduino Uno  A5   A3     A4      11        12      13   A0   5V/3.3V   GND
//ALBIE SAYS: you have to do all of these or it wolnt work.
//Remember to set the pins to suit your display module!


#include <LCDWIKI_GUI.h>  //Core graphics library
#include <LCDWIKI_SPI.h>  //Hardware-specific library

//paramters define. NOTE: don't need to optimise this away, #defines are performed at optimization
#define MODEL ST7796S
#define CS A5
#define CD A3
#define RST A4
#define LED -1  //if you don't need to control the LED pin,you should set it to -1 and set it to 3.3V

//the definiens of hardware spi mode as follow:
//if the IC model is known or the modules is unreadable,you can use this constructed function
LCDWIKI_SPI mylcd(MODEL, CS, CD, RST, LED);  //model,cs,dc,reset,led


//define some colour values
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

//Ratio
#define clksPerMile 1000

//From bildr article: http://bildr.org/2012/08/rotary-encoder-arduino/
//encoder stuff
//these pins can not be changed 2/3 are special pins
#define encoderPin1 2
#define encoderPin2 3

#define inputPinState 4  //change state
#define inputPinIncrease 5
#define inputPinDecrease 6
#define inputPinMode 7

#define modeDist 0
#define modeTime 1
#define modeTillStop 2
#define stateSetup -1
#define stateReady 0
#define stateGoing 1
#define stateFinished 2
byte mode = modeTime;  //current goal type
float target = 100000;  // current goal magnitude

byte state = stateSetup;  //state of the system


volatile int lastEncoded = 0;
volatile long encoderValue = 0;


int lastMSB = 0;
int lastLSB = 0;


void setup() {
  mylcd.Init_LCD();
  mylcd.Fill_Screen(BLACK);

  mylcd.Set_Text_Mode(0);
  mylcd.Set_Rotation(1);
  //display 6 times string
  mylcd.Set_Text_colour(RED);
  mylcd.Set_Text_Back_colour(BLACK);
  mylcd.Set_Text_Size(10);  //the multiplier for the pixel font. Each glyph is a 5x6 grid, and they will be printed with text size pixel space between them



  pinMode(encoderPin1, INPUT_PULLUP);
  pinMode(encoderPin2, INPUT_PULLUP);
  pinMode(inputPinState, INPUT_PULLUP);
  pinMode(inputPinIncrease, INPUT_PULLUP);
  pinMode(inputPinDecrease, INPUT_PULLUP);
  pinMode(inputPinMode, INPUT_PULLUP);

  //call updateEncoder() when any high/low changed seen
  //on interrupt 0 (pin 2), or interrupt 1 (pin 3)
  attachInterrupt(digitalPinToInterrupt(encoderPin1), updateEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(encoderPin2), updateEncoder, CHANGE);
  mode = modeDist;
  setState(stateReady);
}

/*Interface between UI and rotary encoder input handling:
updatencoder is called as interupt when the clk moves, and 
changes encoderValue. main loop checks encoderValue
*/

long inputDelayTime = 0;  //the time in millis at which we will again accept button input, because debouncing is lame
long startTime = 0;       //when the current session started


float colorTheta = 0;  //this is not for actual rotation. its for rainbows!

void loop() {
  //input reading
  bool statePressed = false;
  bool increasePressed = false;
  bool decreasePressed = false;
  bool modePressed = false;
  if (millis() > inputDelayTime) {
    statePressed = !digitalRead(inputPinState);
    increasePressed = !digitalRead(inputPinIncrease);
    decreasePressed = !digitalRead(inputPinDecrease);
    modePressed = !digitalRead(inputPinMode);
    if (statePressed || increasePressed || decreasePressed || modePressed) {
      inputDelayTime = millis() + 1000;
    }
  }
  if (statePressed) {
    setState((state + 1) % 3);
    return;
  }

  switch (state) {
    case stateReady:
      if (modePressed) {
        mode = (mode + 1) % 3;
        target = 100000;

        printTarget();
      }
      if (increasePressed) {
        target *= 2;
        printTarget();
      }
      if (decreasePressed) {
        target /= 2;
        printTarget();
      }


      // statements
      break;
    case stateGoing:
      if (finishChecker()) {
        setState(stateFinished);
        return;
      }
      mylcd.Print_Number_Int(encoderValue, 10, 200, 0, "0", 10);
      if (mode == modeTime) {

        printTime(target + startTime - millis(), 10, 100, 8, false);
      } else {
        printTime(millis() - startTime, 10, 100, 8, false);
      }

      // statements
      break;
    case stateFinished:
      mylcd.Set_Text_colour(nextRainbowColor());
      mylcd.Print("Done!", 100 - round(100 * cos(colorTheta)), 0);
      mylcd.Set_Text_colour(RED);

      // statements
      break;
  }
}
//drawing for when entering state
void setState(byte targState) {
  state = targState;
  switch (targState) {
    case stateReady:
      mylcd.Fill_Screen(BLACK);
      mylcd.Print("READY", 0, 0);
      mylcd.Print("Target:", 0, 100);
      printTarget();
      break;
    case stateGoing:
      mylcd.Fill_Screen(BLACK);
      mylcd.Print("Going", 0, 0);
      encoderValue = 0;
      startTime = millis();

      break;
    case stateFinished:
      long int finTime = millis();
      colorTheta=0;
      mylcd.Fill_Screen(BLACK);
      printTime(finTime - startTime, 10, 100, 8, true);

      mylcd.Set_Text_Size(8);
      mylcd.Print_Number_Int(encoderValue, 10, 180, 1, '0', 10);

      //mylcd.Print_Number_Int(1000 * encoderValue / (finTime - startTime), 10, 180, 1, '0', 10);  //clks per second avg
      mylcd.Set_Text_Size(10);
      break;
  }
}
//formats and prints teh target while in readyup mode
void printTarget() {
  //clear
  mylcd.Print("                ", 0, 200);
  switch (mode) {
    case modeTime:

      printTime(target, 0, 200, 10, false);
      break;
    case modeDist:

      mylcd.Print_Number_Int(target / 100, 0, 200, 1, '0', 10);
      break;
    case modeTillStop:
      mylcd.Print("Free", 0, 200);
      break;
  }
}
//checks if we have met our finish condition
bool finishChecker() {
  // this could all be a one liner, but this is more redable
  if (mode == modeTime && target + startTime <= millis()) {
    return true;
  } else if (mode == modeDist && encoderValue > target/100) {
    return true;
  }

  //TODO: this
  return false;
}
//prints a time in MM:SS format, gives miliseconds in exact mode, which should only be used for an amount of time that has passed.trying to measure time that is happening is to slow to print
//height is standard for font, 6*fontsize pixels tall.
//width is
void printTime(long time, int x0, int y0, int fontsize, bool exact) {
  uint8_t prevSize = mylcd.Get_Text_Size();

  mylcd.Set_Text_Size(fontsize);
  mylcd.Set_Text_Mode(1);
  if (exact) {
    mylcd.Print(".", x0 + 24 * fontsize, y0);
  }

  mylcd.Print(":", x0 + 10 * fontsize, y0);
  mylcd.Set_Text_Mode(0);
  mylcd.Print_Number_Int(time / 60000, x0, y0, 3, '0', 10);
  mylcd.Print_Number_Int((time / 1000) % 60, x0 + 14 * fontsize, y0, 3, '0', 10);

  if (exact) {

    mylcd.Print_Number_Int((time / 1) % 1000, x0 + 29 * fontsize, y0, 4, '0', 10);
  }



  mylcd.Set_Text_Size(prevSize);
}


//Function that gets rainbow colors. The speed of the rainbow is display update speed-dependeaent, cause whatever.
uint16_t nextRainbowColor() {
  colorTheta += 0.1;
  return mylcd.Color_To_565(round(100 - 100 * cos(colorTheta)), round(100 - 100 * cos(colorTheta + 2)), round(100 - 100 * cos(colorTheta + 4)));
}

void updateEncoder() {
  int MSB = digitalRead(encoderPin1);  //MSB = most significant bit
  int LSB = digitalRead(encoderPin2);  //LSB = least significant bit

  int encoded = (MSB << 1) | LSB;          //converting the 2 pin value to single number
  int sum = (lastEncoded << 2) | encoded;  //adding it to the previous encoded value

  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderValue++;
  if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderValue--;

  lastEncoded = encoded;  //store this value for next time
}
