//////////////////////////////////////////////////
              //  LICENSE  //
//////////////////////////////////////////////////
#pragma region LICENSE
/*
  ************************************************************************************
  * MIT License
  *
  * Copyright (c) 2025 Crunchlabs LLC (LabelMaker Code)

  * Permission is hereby granted, free of charge, to any person obtaining a copy
  * of this software and associated documentation files (the "Software"), to deal
  * in the Software without restriction, including without limitation the rights
  * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  * copies of the Software, and to permit persons to whom the Software is furnished
  * to do so, subject to the following conditions:
  *
  * The above copyright notice and this permission notice shall be included in all
  * copies or substantial portions of the Software.
  *
  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
  * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
  * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
  * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
  * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
  * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
  *
  ************************************************************************************
*/
#pragma endregion LICENSE

//////////////////////////////////////////////////
              //  LIBRARIES  //
//////////////////////////////////////////////////
#pragma region LIBRARIES
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Stepper.h>
#include <ezButton.h>
#include <Servo.h>

#pragma endregion LIBRARIES

//////////////////////////////////////////////////
          //  PINS AND PARAMETERS  //
//////////////////////////////////////////////////
#pragma region PINS AND PARAMS

LiquidCrystal_I2C lcd(0x27, 16, 2);  // Set the LCD address to 0x27 for a 16x2 display

ezButton button1(14); //joystick button handler
#define INIT_MSG "Initializing..." // Text to display on startup
#define MODE_NAME "   LABELMAKER   " //these are variables for the text which is displayed in different menus. 
#define PRINT_CONF "  PRINT LABEL?  " //try changing these, or making new ones and adding conditions for when they are used
#define PRINTING "    PRINTING    " // NOTE: this text must be 16 characters or LESS in order to fit on the screen correctly
#define MENU_CLEAR ":                " //this one clears the menu for editing


//text variables
int x_scale = 230;//these are multiplied against the stored coordinate (between 0 and 4) to get the actual number of steps moved
int y_scale = 230;//for example, if this is 230(default), then 230(scale) x 4(max coordinate) = 920 (motor steps)
int scale = x_scale;
int space = x_scale * 5; //space size between letters (as steps) based on X scale in order to match letter width
//multiplied by 5 because the scale variables are multiplied against coordinates later, while space is just fed in directly, so it needs to be scaled up by 5 to match
int row_spacing = y_scale * 1.5; //vertical spacing between rows (based on character height * 3.5 y-scale factor)


// Joystick setup
const int joystickXPin = A2;  // Connect the joystick X-axis to this analog pin
const int joystickYPin = A1;  // Connect the joystick Y-axis to this analog pin
const int joystickButtonThreshold = 200;  // Adjust this threshold value based on your joystick

// Menu parameters
const char alphabet[] = "_ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!?,.#@"; //alphabet menu
int alphabetSize = sizeof(alphabet) - 1;
String textRow1;  // Store the label text for first row
String textRow2;  // Store the label text for second row
int currentRow = 0;  // Track which row is being edited (0 = row 1, 1 = row 2)

int currentCharacter = 0; //keep track of which character is currently displayed under the cursor
int cursorPosition = 0; //keeps track of the cursor position (left to right) on the screen
int currentPage = 0; //keeps track of the current page for menus
const int charactersPerPage = 16; //number of characters that can fit on one row of the screen

// Stepper motor parameters
const int stepCount = 200;
const int stepsPerRevolution = 2048;

// initialize the stepper library for both steppers:
Stepper xStepper(stepsPerRevolution, 6, 8, 7, 9);
Stepper yStepper(stepsPerRevolution, 2, 4, 3, 5); 

int xPins[4] = {6, 8, 7, 9};  // pins for x-motor coils
int yPins[4] = {2, 4, 3, 5};    // pins for y-motor coils

//Servo
const int SERVO_PIN  = 13;
Servo servo;
int angle = 30; // the current angle of servo motor


// Creates states to store what the current menu and joystick states are
// Decoupling the state from other functions is good because it means the sensor / screen aren't hardcoded into every single action and can be handled at a higher level
enum State { MainMenu, Editing, PrintConfirmation, Printing };
State currentState = MainMenu;
State prevState = Printing;

enum jState {LEFT, RIGHT, UP, DOWN, MIDDLE, UPRIGHT, UPLEFT, DOWNRIGHT, DOWNLEFT};
jState joyState = MIDDLE;
jState prevJoyState = MIDDLE;

boolean pPenOnPaper = false; // pen on paper in previous cycle
int lineCount = 0;

int xpos = 0;
int ypos = 0;
const int posS = 2;
const int posM = 7;
const int posL = 12;
bool joyUp;
bool joyDown;
bool joyLeft;
bool joyRight;
int button1State;
bool button1Handled = false;  // Track if button press has been handled in current cycle
int joystickX;
int joystickY;
#pragma endregion PINS AND PARAMS

//////////////////////////////////////////////////
           //  CHARACTER VECTORS  //
//////////////////////////////////////////////////
#pragma region CHARACTER VECTORS
const uint8_t vector[63][14] = {
  /*
    encoding works as follows:
    ones     = y coordinate;
    tens     = x coordinate;
    hundreds = draw/don't draw ..
    200      = end
    222      = plot point
    !! for some reason leading zeros cause problems !!
  */
  {  0,  124,  140,  32,  112,   200,  200,  200,  200,  200,  200,  200,  200,  200}, //my A character
  {  0,  104,  134,  132,    2,  142,  140,  100,  200,  200,  200,  200,  200,  200}, /*B*/ // the 2 was originally 002, not sure why
  { 41,  130,  110,  101,  103,  114,  134,  143,  200,  200,  200,  200,  200,  200}, /*C*/
  {  0,  104,  134,  143,  141,  130,  100,  200,  200,  200,  200,  200,  200,  200}, /*D*/
  { 40,  100,  104,  144,   22,  102,  200,  200,  200,  200,  200,  200,  200,  200}, /*E*/
  {  0,  104,  144,   22,  102,  200,  200,  200,  200,  200,  200,  200,  200,  200}, /*F*/
  { 44,  104,  100,  140,  142,  122,  200,  200,  200,  200,  200,  200,  200,  200}, /*G*/
  {  0,  104,    2,  142,   44,  140,  200,  200,  200,  200,  200,  200,  200,  200}, /*H*/
  {  0,  104,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200}, /*I*/
  {  1,  110,  130,  141,  144,  200,  200,  200,  200,  200,  200,  200,  200,  200}, /*J*/
  {  0,  104,    2,  142,  140,   22,  144,  200,  200,  200,  200,  200,  200,  200}, /*K*/
  { 40,  100,  104,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200}, /*L*/
  {  0,  104,  122,  144,  140,  200,  200,  200,  200,  200,  200,  200,  200,  200}, /*M */
  {  0,  104,  140,  144,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200}, /*N*/
  { 10,  101,  103,  114,  134,  143,  141,  130,  110,  200,  200,  200,  200,  200}, /*O*/
  {  0,  104,  144,  142,  102,  200,  200,  200,  200,  200,  200,  200,  200,  200}, /*P*/
  {  0,  104,  144,  142,  120,  100,   22,  140,  200,  200,  200,  200,  200,  200}, /*Q*/
  {  0,  104,  144,  142,  102,   22,  140,  200,  200,  200,  200,  200,  200,  200}, /*R*/
  {  0,  140,  142,  102,  104,  144,  200,  200,  200,  200,  200,  200,  200,  200}, /*S*/
  { 20,  124,    4,  144,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200}, /*T*/
  {  4,  101,  110,  130,  141,  144,  200,  200,  200,  200,  200,  200,  200,  200}, /*U*/
  {  4,  120,  144,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200}, /*V*/
  {  4,  100,  122,  140,  144,  200,  200,  200,  200,  200,  200,  200,  200,  200}, /*W*/
  {  0,  144,    4,  140,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200}, /*X*/
  {  4,  122,  144,   22,  120,  200,  200,  200,  200,  200,  200,  200,  200,  200}, /*Y*/
  {  4,  144,  100,  140,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200}, /*Z*/
  {  0,  104,  144,  140,  100,  144,  200,  200,  200,  200,  200,  200,  200,  200}, /*0*/
  {  0,  140,   20,  124,  104,  200,  200,  200,  200,  200,  200,  200,  200,  200}, /*1*/
  {  4,  144,  142,  102,  100,  140,  200,  200,  200,  200,  200,  200,  200,  200}, /*2*/
  {  0,  140,  144,  104,   12,  142,  200,  200,  200,  200,  200,  200,  200,  200}, /*3*/
  { 20,  123,   42,  102,  104,  200,  200,  200,  200,  200,  200,  200,  200,  200}, /*4*/
  {  0,  140,  142,  102,  104,  144,  200,  200,  200,  200,  200,  200,  200,  200}, /*5*/
  {  2,  142,  140,  100,  104,  144,  200,  200,  200,  200,  200,  200,  200,  200}, /*6*/
  {  0,  144,  104,   12,  132,  200,  200,  200,  200,  200,  200,  200,  200,  200}, /*7*/
  {  0,  140,  144,  104,  100,    2,  142,  200,  200,  200,  200,  200,  200,  200}, /*8*/
  {  0,  140,  144,  104,  102,  142,  200,  200,  200,  200,  200,  200,  200,  200}, /*9*/
  { 200, 200,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200}, /* */
  { 200, 200,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200}, /* */
  {  0,  144,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200}, /*/*/
  {  0,  102,  124,  142,  140,   42,  102,    4,  103,   44,  143,  200,  200,  200}, /*Ä*/
  {  0,  102,  142,  140,  100,    2,   14,  113,   34,  133,  200,  200,  200,  200}, /*Ö*/
  {  4,  100,  140,  144,   14,  113,   34,  133,  200,  200,  200,  200,  200,  200}, /*Ü*/
  {  0,  111,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200}, /*,*/
  {  2,  142,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200}, /*-*/
  {  0,  222,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200}, /*.*/
  {  0,  222,    1,  104,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200}, /*!*/
  {  20, 222,   21,  122,  142,  144,  104,  200,  200,  200,  200,  200,  200,  200}, /*?*/ 
  {  0,  104,  134,  133,  122,  142,  140,  110,  200,  200,  200,  200,  200,  200}, /*ß*/
  {  23, 124,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200}, /*'*/
  {  42, 120,  100,  101,  123,  124,  104,  103,  130,  140,  200,  200,  200,  200}, /*&*/
  {  2,  142,   20,  124,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200}, /*+*/
  {  21, 222,   23,  222,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200}, /*:*/
  {  10, 121,   22,  222,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200}, /*;*/
  {  14, 113,   33,  134,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200}, /*"*/
  {  10, 114,   34,  130,   41,  101,    3,  143,  200,  200,  200,  200,  200,  200}, /*#*/
  {  34, 124,  120,  130,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200}, /*(*/
  {  10, 120,  124,  114,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200}, /*)*/
  {  1,  141,   43,  103,  200,  200,  200,  200,  200,  200,  200,  200,  200,  200}, /*=*/
  {  31, 133,  113,  111,  141,  144,  104,  100,  140,  200,  200,  200,  200,  200}, /*@*/
  {  2,  142,   20,  124,    4,  140,  0,    144,  200,  200,  200,  200,  200,  200}, /***/
  {  0,  140,  144,  104,  100,   12,  113,   33,  132,   31,  111,  200,  200,  200}, /*} Smiley*/
  {  0,  140,  144,  104,  100,   13,  222,   33,  222,   32,  131,  111,  112,  132}, /*~ Open mouth Smiley*/
  {  20, 142,  143,  134,  123,  114,  103,  102,  120,  200,  200,  200,  200,  200} /*$ Heart*/
};
#pragma endregion CHARACTER VECTORS

//////////////////////////////////////////////////
              //  S E T U P  //
//////////////////////////////////////////////////
#pragma region SETUP
void setup() {
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print(INIT_MSG);  // print start up message

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(9600);

  button1.setDebounceTime(50);  //debounce prevents the joystick button from triggering twice when clicked

  servo.attach(SERVO_PIN);  // attaches the servo on pin 9 to the servo object
  servo.write(angle);

  plot(false);  //servo to tape surface so pen can be inserted

  // set the speed of the motors
  yStepper.setSpeed(12);  // set first stepper speed (these should stay the same)
  xStepper.setSpeed(10);  // set second stepper speed (^ weird stuff happens when you push it too fast)

  penUp();      //ensure that the servo is lifting the pen carriage away from the tape
  homeYAxis();  //lower the Y axis all the way to the bottom

  ypos = 0;
  xpos = 0;

  releaseMotors();
  lcd.clear();
}
#pragma endregion SETUP

//////////////////////////////////////////////////
                //  L O O P  //
//////////////////////////////////////////////////
#pragma region LOOP
void loop() {

  button1.loop();
  button1State = button1.getState();
  
  // Reset button handled flag when button is released
  if (button1.isReleased()) {
    button1Handled = false;
  }

  joystickX = analogRead(joystickXPin);
  joystickY = analogRead(joystickYPin);
  joyUp = joystickY < (512 - joystickButtonThreshold);
  joyDown = joystickY > (512 + joystickButtonThreshold);
  joyLeft = joystickX < (512 - joystickButtonThreshold);
  joyRight = joystickX > (512 + joystickButtonThreshold);

  switch (currentState) {  //state machine that determines what to do with the input controls based on what mode the device is in

    case MainMenu:
      {
        if (prevState != MainMenu) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(MODE_NAME);
          lcd.setCursor(0, 1);
          lcd.print("      START     ");
          cursorPosition = 5;
          prevState = MainMenu;
        }

        lcd.setCursor(cursorPosition, 1);

        if (millis() % 600 < 400) {  // Blink every 500 ms
          lcd.print(">");
        } else {
          lcd.print(" ");
        }

        if (button1.isPressed()) {  //handles clicking options in text size setting
          lcd.clear();
          currentState = Editing;
          prevState = MainMenu;
        }
      }
      break;

    case Editing:
      //in the editing mode, joystick directional input adds and removes characters from the string, while up and down changes characters
      //first joystick click moves to second row, second click goes to print confirmation

      // Editing mode
      if (prevState != Editing) {
        lcd.clear();
        currentRow = 0;  // Reset to first row when entering editing mode
        prevState = Editing;
      }
      
      {
          // Display both rows on LCD (only update if not processing joystick input to avoid flicker)
        static unsigned long lastDisplayUpdate = 0;
        bool joystickActive = joyLeft || joyRight || joyUp || joyDown;
        
        if (!joystickActive || (millis() - lastDisplayUpdate > 100)) {
          lcd.setCursor(0, 0);
          lcd.print(":");
          lcd.setCursor(1, 0);
          // Truncate to fit on screen (16 chars max per row, accounting for ":")
          if (textRow1.length() > 15) {
            lcd.print(textRow1.substring(0, 15));
          } else {
            lcd.print(textRow1);
          }
          
          lcd.setCursor(0, 1);
          lcd.print(":");  // Show ":" at beginning of second row too
          lcd.setCursor(1, 1);
          // Truncate to fit on screen (16 chars max per row, accounting for ":")
          if (textRow2.length() > 15) {
            lcd.print(textRow2.substring(0, 15));
          } else {
            lcd.print(textRow2);
          }
          lastDisplayUpdate = millis();
        }
      }

      // Check if the joystick is moved left (backspace) or right (add space) - check these first
      if (joyLeft) {
        // LEFT (backspace)
        String* currentText = (currentRow == 0) ? &textRow1 : &textRow2;
        if (currentText->length() > 0) {
          currentText->remove(currentText->length() - 1);
          // Force display update after backspace
          lcd.setCursor(0, 0);
          lcd.print(":                ");
          lcd.setCursor(1, 0);
          if (textRow1.length() > 15) {
            lcd.print(textRow1.substring(0, 15));
          } else {
            lcd.print(textRow1);
          }
          lcd.setCursor(0, 1);
          lcd.print(":                ");
          lcd.setCursor(1, 1);
          if (textRow2.length() > 15) {
            lcd.print(textRow2.substring(0, 15));
          } else {
            lcd.print(textRow2);
          }
        }
        delay(250);  // Delay to prevent rapid multiple presses

      } else if (joyRight) {  //RIGHT adds a space or character to the label
        String* currentText = (currentRow == 0) ? &textRow1 : &textRow2;
        if (currentCharacter == 0) {
          *currentText += ' ';  //add a space if the character is _
        } else {
          *currentText += alphabet[currentCharacter];  //add the current character to the text
          currentCharacter = 0;
        }
        // Force display update after adding character
        lcd.setCursor(0, 0);
        lcd.print(":                ");
        lcd.setCursor(1, 0);
        if (textRow1.length() > 15) {
          lcd.print(textRow1.substring(0, 15));
        } else {
          lcd.print(textRow1);
        }
        lcd.setCursor(0, 1);
        lcd.print(":                ");
        lcd.setCursor(1, 1);
        if (textRow2.length() > 15) {
          lcd.print(textRow2.substring(0, 15));
        } else {
          lcd.print(textRow2);
        }
        delay(250);  // Delay to prevent rapid multiple presses

      } else if (joyUp) {  //UP (previous character)
        if (currentCharacter > 0) {
          currentCharacter--;
        }
        delay(250);  // Delay to prevent rapid scrolling

      } else if (joyDown) {  //DOWN (next character)
        if (currentCharacter < (alphabetSize - 1)) {
          currentCharacter++;  //increment character value
        }
        delay(250);  // Delay to prevent rapid scrolling
      } else {
        // Display current character being selected (blinking cursor) - only when no joystick input
        String* currentText = (currentRow == 0) ? &textRow1 : &textRow2;
        // Cursor position is length of current text + 1 (accounting for ":" at position 0 on both rows)
        int cursorCol = currentText->length() + 1;
        if (cursorCol > 16) cursorCol = 16;  // Don't exceed screen width
        
        lcd.setCursor(cursorCol, currentRow);
        if (millis() % 600 < 450) {
          lcd.print(alphabet[currentCharacter]);
        } else {
          lcd.print(" ");
        }
      }

      // Check for button press - only handle once per press
      if (button1.isPressed() && !button1Handled) {
        button1Handled = true;  // Mark as handled to prevent multiple triggers
        if (currentRow == 0) {
          // First click: move to second row
          currentRow = 1;
          currentCharacter = 0;  // Reset character selection
          delay(200);  // Debounce delay
        } else {
          // Second click: go to print confirmation
          //lcd.clear();
          currentState = PrintConfirmation;
          prevState = Editing;
          button1Handled = false;  // Reset flag immediately
        }
      }

      break;

    case PrintConfirmation:
      // Print confirmation mode
      // Initialize screen when coming from Editing
      if (prevState == Editing) {
        lcd.clear();  // Clear screen first
        delay(100);  // Small delay to ensure clear completes
        lcd.setCursor(0, 0);    //move cursor to the first line
        lcd.print(PRINT_CONF);  //print menu text
        lcd.setCursor(0, 1);    // move cursor to the second line
        lcd.print("   YES     NO   ");
        lcd.setCursor(2, 1);
        cursorPosition = 2;
        prevState = PrintConfirmation;  // Update prevState after initialization
        button1Handled = true;  // Set to true initially to prevent immediate trigger from button still being pressed
        delay(200);  // Delay to allow button to be released
      }

      //the following two if statements help move the blinking cursor from one option to the other.
      if (joyLeft) {  //left
        lcd.setCursor(0, 1);
        lcd.print("   YES     NO   ");
        lcd.setCursor(2, 1);
        cursorPosition = 2;
        delay(200);
      } else if (joyRight) {  //right
        lcd.setCursor(0, 1);
        lcd.print("   YES     NO   ");
        lcd.setCursor(10, 1);
        cursorPosition = 10;
        delay(200);
      }

      lcd.setCursor(cursorPosition, 1);

      if (millis() % 600 < 400) {  // Blink every 500 ms
        lcd.print(">");
      } else {
        lcd.print(" ");
      }

      if (button1.isPressed() && !button1Handled) {    //handles clicking options in print confirmation
        button1Handled = true;  // Mark as handled
        if (cursorPosition == 2) {  //proceed to printing if clicking yes
          lcd.clear();
          currentState = Printing;
          prevState = PrintConfirmation;

        } else if (cursorPosition == 10) {  //return to editing if you click no
          lcd.clear();
          currentState = Editing;
          prevState = PrintConfirmation;
          button1Handled = false;  // Reset for next time
        }
      }

      break;

    case Printing:
      // Printing mode
      if (prevState == PrintConfirmation) {
        lcd.setCursor(0, 0);
        lcd.print(PRINTING);  //update screen
      }

      // ----------------------------------------------- plot text
      // Ensure we start from the correct position
      xpos = 0;
      ypos = 0;
      
      plotText(textRow1, textRow2, xpos, ypos);

      // Move to new line after printing
      line(xpos + space, 0, 0);  // move to new line
      xpos = 0;
      ypos = 0;

      //textRow1 = "";
      //textRow2 = "";
      homeYAxis();
      releaseMotors();
      lcd.clear();
      currentState = Editing;
      prevState = Printing;

      break;
  }
}
#pragma endregion LOOP

//////////////////////////////////////////////////
              // FUNCTIONS  //
//////////////////////////////////////////////////
#pragma region FUNCTIONS
void plotText(String &row1, String &row2, int x, int y) {  //takes in two row strings and plots them
  
  // Check if both rows have content
  bool hasRow1 = row1.length() > 0;
  bool hasRow2 = row2.length() > 0;
  bool twoRows = hasRow1 && hasRow2;
  
  // Ensure we start from the correct position
  xpos = x;
  ypos = y;
  
  if (twoRows) {
    // Store original scales
    int orig_x_scale = x_scale;
    int orig_y_scale = y_scale;
    int orig_space = space;
    int orig_scale = scale;
    
    // Scale to half size
    x_scale = orig_x_scale / 2;
    y_scale = orig_y_scale / 2;
    space = orig_space / 2;
    scale = orig_scale / 2;
    
    // Calculate half-sized character height
    int halfCharHeight = y_scale * 4 * 3.5;  // Half-sized character height
    
    // Position row1 at the top (y position)
    // Characters draw upward from their base Y coordinate
    int row1_y = y + halfCharHeight + row_spacing;
    
    // Position row2 below row1 with proper spacing
    // Move down by half character height plus spacing
    int row2_y = y;
    
    // Plot row1 at top
    plotTextRow(row1, x, row1_y);
    
    // Move to row2 position (back to start X, down to row2 Y)
    line(x, row2_y, 0);
    
    // Plot row2 below row1
    plotTextRow(row2, x, row2_y);
    
    // Restore original scales
    x_scale = orig_x_scale;
    y_scale = orig_y_scale;
    space = orig_space;
    scale = orig_scale;
    
  } else {
    // Single row - use normal size
    if (hasRow1) {
      plotTextRow(row1, x, y);
    }
    
    if (hasRow2) {
      // Calculate second row Y position (down by row_spacing)
      int row2_y = y - row_spacing;
      
      // If row1 was drawn, move to row2 position, otherwise start at row2 position
      if (hasRow1) {
        // Move back to start X position and down to row2 Y position without drawing
        line(x, row2_y, 0);
      } else {
        // If row1 is empty, just set position to row2 start
        xpos = x;
        ypos = row2_y;
      }
      
      // Draw second row
      plotTextRow(row2, x, row2_y);
    }
  }
  
  releaseMotors();
}

void plotTextRow(String &str, int x, int y) {  //helper function to plot a single row of text
  int pos = 0;
  for (int i = 0; i < str.length(); i++) {  //for each letter in the string (expressed as "while i is less than string length")
    char c = char(str.charAt(i));           //store the next character to plot on it's own
    if (byte(c) != 195) {
      if (c == ' ') {  //if it's a space, add a space.
        pos += space;
      } else {
        plotCharacter(c, x + pos, y);
        pos += space;  //scale is multiplied by 4 here to convert it to steps (because it normally get's multiplied by a coordinate with a max of 4)
        if (c == 'I' || c == 'i') pos -= (scale * 4) / 1.1;
        if (c == ',') pos -= (scale * 4) / 1.2;
      }
    }
  }
}

void plotCharacter(char c, int x, int y) {  //this receives info from plotText for which character to plot,
  // first it does some logic to make specific tweaks depending on the character, so some characters need more space, others less,
  // and some we even want to swap (in the case of space, we're swapping _ (underscore) and space so that we have something to show on the screen)

  // and once we've got it all worked out right, this function passes the coordinates from that character though line() function to draw it

  Serial.print(uint8_t(c));  //print the received character to monitor
  Serial.print(">");

  //the following if statements handle character specific changes by shifting / swapping prior to drawing
  uint8_t character = 38;
  if (uint8_t(c) > 64 and uint8_t(c) < 91) {  //A...Z
    character = uint8_t(c) - 65;
  }
  if (uint8_t(c) > 96 and uint8_t(c) < 123) {  //A...Z
    character = uint8_t(c) - 97;
  }
  if (uint8_t(c) > 47 and uint8_t(c) < 58) {  //0...9
    character = uint8_t(c) - 22;
  }
  if (uint8_t(c) == 164 || uint8_t(c) == 132) {  //ä,Ä
    character = 39;
  }
  if (uint8_t(c) == 182 || uint8_t(c) == 150) {  //ö,Ö
    character = 40;
  }
  if (uint8_t(c) == 188 || uint8_t(c) == 156) {  //ü,Ü
    character = 41;
  }
  if (uint8_t(c) == 44) {  // ,
    character = 42;
  }
  if (uint8_t(c) == 45) {  // -
    character = 43;
  }
  if (uint8_t(c) == 46) {  // .
    character = 44;
  }
  if (uint8_t(c) == 33) {  // !
    character = 45;
  }
  if (uint8_t(c) == 63) {  // ?
    character = 46;
  }
  if (uint8_t(c) == 123) { /*{ ß*/
    character = 47;
  }
  if (uint8_t(c) == 39) { /*'*/
    character = 48;
  }
  if (uint8_t(c) == 38) { /*&*/
    character = 49;
  }
  if (uint8_t(c) == 43) { /*+*/
    character = 50;
  }
  if (uint8_t(c) == 58) { /*:*/
    character = 51;
  }
  if (uint8_t(c) == 59) { /*;*/
    character = 52;
  }
  if (uint8_t(c) == 34) { /*"*/
    character = 53;
  }
  if (uint8_t(c) == 35) { /*#*/
    character = 54;
  }
  if (uint8_t(c) == 40) { /*(*/
    character = 55;
  }
  if (uint8_t(c) == 41) { /*)*/
    character = 56;
  }
  if (uint8_t(c) == 61) { /*=*/
    character = 57;
  }
  if (uint8_t(c) == 64) { /*@*/
    character = 58;
  }
  if (uint8_t(c) == 42) { /***/
    character = 59;
  }
  if (uint8_t(c) == 125) { /*} Smiley*/
    character = 60;
  }
  if (uint8_t(c) == 126) { /*~ Open mouth Smiley*/
    character = 61;
  }
  if (uint8_t(c) == 36) { /*$ Heart*/
    character = 62;
  }
  Serial.print("letter: ");
  Serial.println(c);
  for (int i = 0; i < 14; i++) {  // go through each vector of the character

    int v = vector[character][i];
    if (v == 200) {  // no more vectors in this array

      break;
    }
    if (v == 222) {  // plot single point
      plot(true);
      delay(50);
      plot(false);
    } else {
      int draw = 0;
      if (v > 99) {
        draw = 1;
        v -= 100;
      }
      int cx = v / 10;       // get x ...
      int cy = v - cx * 10;  // and y

      //if(cx > 0) cx = 1;

      // 1: Normalize
      int x_start = x;
      int x_end = x + cx * x_scale;
      int y_start = y;
      int y_end = y + cy * y_scale * 3.5;  //we multiply by 3.5 here to equalize the Y output to match X,
      //this is because the Y lead screw covers less distance per-step than the X motor wheel (about 3.5 times less haha)
      bool switched = false;

      Serial.print("Scale: ");
      Serial.print(scale);
      Serial.print("  ");
      Serial.print("X Goal: ");
      Serial.print(x_end);
      Serial.print("  ");
      Serial.print("Y Goal: ");
      Serial.print(y_end);
      Serial.print("  ");
      Serial.print("Draw: ");
      Serial.println(draw);

      line(x_end, y_end, draw);
    }
  }
}

void line(int newx, int newy, bool drawing) {
  //this function is an implementation of bresenhams line algorithm
  //this algorithm basically finds the slope between any two points, allowing us to figure out how many steps each motor should do to move smoothly to the target
  //in order to do this, we give this function our next X (newx) and Y (newy) coordinates, and whether the pen should be up or down (drawing)

  if (drawing < 2) {  //checks if we should be drawing and puts the pen up or down based on that.
    plot(drawing);    // dashed: 0= don't draw / 1=draw / 2... = draw dashed with variable dash width
  } else {
    plot((stepCount / drawing) % 2);  //can do dashed lines, but for now this isn't doing anything since we're only sending 0 or 1.
  }

  int i;
  long over = 0;

  long dx = newx - xpos;  //calculate the difference between where we are (xpos) and where we want to be (newx)
  long dy = newy - ypos;
  int dirx = dx > 0 ? -1 : 1;  //this is called a ternary operator, it's basically saying: if dx is greater than 0, then dirx = -1, if dx is less than or equal to 0, dirx = 1.
  int diry = dy > 0 ? 1 : -1;  //this is called a ternary operator, it's basically saying: if dy is greater than 0, then diry = 1, if dy is less than or equal to 0, diry = -1.
  //the reason one of these ^ is inverted logic (1/-1) is due to the direction these motors rotate in the system.

  dx = abs(dx);  //normalize the dx/dy values so that they are positive.
  dy = abs(dy);  //abs() is taking the "absolute value" - basically it removes the negative sign from negative numbers

  //the following nested If statements check which change is greater, and use that to determine which coordinate (x or y) get's treated as the rise or the run in the slope calculation
  //we have to do this because technically bresenhams only works for the positive quandrant of the cartesian coordinate grid,
  // so we are just flipping the values around to get the line moving in the correct direction relative to it's current position (instead of just up an to the right)
  if (dx > dy) {
    over = dx / 2;
    for (i = 0; i < dx; i++) {  //for however much our current position differs from the target,
      xStepper.step(dirx);      //do a step in that direction (remember, dirx is always going to be either 1 or -1 from the ternary operator above)

      // Serial.print("Xsteps: ");
      // Serial.print(dirx);
      // Serial.print("  ");

      over += dy;
      if (over >= dx) {
        over -= dx;

        // Serial.print("Ysteps: ");
        // Serial.println(diry);

        yStepper.step(diry);
      }
      //delay(1);
    }
  } else {
    over = dy / 2;
    for (i = 0; i < dy; i++) {
      yStepper.step(diry);
      // Serial.print("Ysteps: ");
      // Serial.print(diry);
      // Serial.print("  ");
      over += dx;
      if (over >= dy) {
        over -= dy;
        // Serial.print("Xsteps: ");
        // Serial.println(dirx);
        xStepper.step(dirx);
      }
      //delay(1);
    }
  }
  xpos = newx;  //store positions
  ypos = newy;  //store positions
}

void plot(boolean penOnPaper) {  //used to handle lifting or lowering the pen on to the tape
  if (penOnPaper) {              //if the pen is already up, put it down
    angle = 80;
  } else {  //if down, then lift up.
    angle = 25;
  }
  servo.write(angle);                        //actuate the servo to either position.
  if (penOnPaper != pPenOnPaper) delay(50);  //gives the servo time to move before jumping into the next action
  pPenOnPaper = penOnPaper;                  //store the previous state.
}

void penUp() {  //singular command to lift the pen up
  servo.write(25);
}

void penDown() {  //singular command to put the pen down
  servo.write(80);
}

void releaseMotors() {
  for (int i = 0; i < 4; i++) {  //deactivates all the motor coils
    digitalWrite(xPins[i], 0);   //just picks each motor pin and send 0 voltage
    digitalWrite(yPins[i], 0);
  }
  plot(false);
}

void homeYAxis() {
  yStepper.step(-3000);  //lowers the pen holder to it's lowest position.
}

void resetScreen() {
  lcd.clear();          // clear LCD
  lcd.setCursor(0, 0);  // set cursor to row 0 column 0
  lcd.print(": ");
  lcd.setCursor(1, 0);  //move cursor down to row 1 column 0
  cursorPosition = 1;
}
#pragma endregion FUNCTIONS

//////////////////////////////////////////////////
               //  END CODE  //
//////////////////////////////////////////////////
