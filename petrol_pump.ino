/*
 * Toy petrol pump
 * Uses a 16x2 LCD display
 */

#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Constants
#define BUTTON_ADC_PIN           A0  // A0 is the button ADC input
#define LCD_BACKLIGHT_PIN        10  // D10 controls LCD backlight
#define RIGHT_10BIT_ADC           0  // right
#define UP_10BIT_ADC            145  // up
#define DOWN_10BIT_ADC          329  // down
#define LEFT_10BIT_ADC          505  // left
#define SELECT_10BIT_ADC        741  // right
#define BUTTONHYSTERESIS         10  // hysteresis for valid button sensing window
#define BUTTON_NONE               0  // 
#define BUTTON_RIGHT              1  // 
#define BUTTON_UP                 2  // 
#define BUTTON_DOWN               3  // 
#define BUTTON_LEFT               4  // 
#define BUTTON_SELECT             5  // 

// Global variables
byte buttonJustPressed  = false;         //this will be true after a ReadButtons() call if triggered
byte buttonJustReleased = false;         //this will be true after a ReadButtons() call if triggered
byte buttonWas          = BUTTON_NONE;   //used by ReadButtons() for detection of button events
char welcomeMsg[]       = "Welcome!";
float pricePerLitre     = 117.9;
byte inFuelMode         = false;
byte wasInFuelMode      = false;
float flowRate          = 0.01;
unsigned long timeOut   = 50000;

byte topLeftPumpSymbol[8] = {
    0b00000,
    0b00000,
    0b00111,        
    0b01111,
    0b11111,
    0b10101,
    0b11111,
    0b00000
};
byte topRightPumpSymbol[8] = {
    0b00011,
    0b00011,
    0b11111,        
    0b11111,
    0b11111,
    0b10101,
    0b11111,
    0b00011
};
byte bottomLeftPumpSymbol[8] = {
    0b00000,
    0b00000,
    0b01111,
    0b11011,
    0b11011,
    0b11111,
    0b11111,
    0b11111
};
byte bottomRightPumpSymbol[8] = {
    0b00011,
    0b00011,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111
};
byte perLitreSymbol[8] = {
    0b11000,
    0b10100,
    0b11101,      
    0b10010,
    0b00101,
    0b01001,
    0b10001,
    0b00001
};

void setup() {
  // Set the LCD's number of columns and rows respectively
  lcd.begin(16,2);

  // Display the pump's welcome message
  displayWelcome(welcomeMsg, pricePerLitre);
}

void loop() {

  byte WELCOME = 0;
  byte STARTED = 1;
  byte STOPPED = 2;

  float numLitresServed = 0;
  byte state = WELCOME;
  bool welcomeAlreadyShown = true;
  unsigned long stopTime = 0;
  
  while (true) {
    byte button = readButtons();
    bool nozzleTriggered = (button == BUTTON_LEFT);

    if (state == WELCOME) {
      if (nozzleTriggered) {
        numLitresServed = 0;  // just started fuelling up
        initaliseCostDisplay();
        state = STARTED;
      } else {
        if (!welcomeAlreadyShown) {
          displayWelcome(welcomeMsg, pricePerLitre);
          welcomeAlreadyShown = true;
        }
      }
    } else if (state == STARTED) {
      if (nozzleTriggered) {
        welcomeAlreadyShown = false;
        numLitresServed += flowRate;  // increment the amount of litres served
        displayLitresAndCost(numLitresServed, pricePerLitre);  // display cost
      } else {
        state = STOPPED;
        stopTime = 0;
      }
    } else {  // STOPPED state
      stopTime += 1;
      if (stopTime >= timeOut) {
        state = WELCOME;  // reset
      } else if (nozzleTriggered) {
        state = STARTED;
      }
    }
  }
}

char* roundToOneDecimalPlace(float x) {
  char buffer[7];
  return(dtostrf(x, 5, 1, buffer));  // 5 digits, 1 decimal place
}

char* roundToTwoDecimalPlaces(float x) {
  char buffer[8];
  return(dtostrf(x, 5, 2, buffer));  // 5 digits, 1 decimal place
}

void initaliseCostDisplay() {
  // Clear the display
  lcd.clear();

  // Top line
  lcd.setCursor(0,0);
  lcd.write("Litres   ");

  // Bottom line
  lcd.setCursor(0,1);
  lcd.write("Price   ");
  lcd.write(0b11101101);
}

void displayLitresAndCost(float numLitres, float pricePerLitreInPence) {
  float priceInPounds = numLitres * pricePerLitreInPence / 100;
  displayCost(numLitres, priceInPounds);
}

void displayCost(float numLitres, float priceInPounds) {
  // Top line
  lcd.setCursor(9,0);
  lcd.write(roundToTwoDecimalPlaces(numLitres));

  // Bottom line
  lcd.setCursor(9,1);
  lcd.write(roundToTwoDecimalPlaces(priceInPounds));
}

// Display the welcome message
void displayWelcome(char* msg, float pricePerLitre) {

  // Clear the LCD display
  lcd.clear();

  // Build the petrol pump symbols
  lcd.createChar(0, topLeftPumpSymbol);
  lcd.createChar(1, topRightPumpSymbol);
  lcd.createChar(2, bottomLeftPumpSymbol);
  lcd.createChar(3, bottomRightPumpSymbol);
  lcd.createChar(4, perLitreSymbol);
  lcd.setCursor(0,0);

  // Top line
  lcd.write(byte(0));
  lcd.write(byte(1));
  lcd.setCursor(3,0);
  lcd.write(msg);

  // Bottom line
  lcd.setCursor(0,1);
  lcd.write(byte(2));
  lcd.write(byte(3));
  lcd.setCursor(3,1);
  lcd.print("Petrol ");  // Display the price per litre
  lcd.print(roundToOneDecimalPlace(pricePerLitre));
  lcd.write(byte(4));
}

// Set up the backlight.
void setBacklight() {
  digitalWrite(LCD_BACKLIGHT_PIN, HIGH);  //backlight control pin D3 is high (on)
  pinMode(LCD_BACKLIGHT_PIN, OUTPUT);     //D3 is an output
}

// This code comes directly from:
// http://linksprite.com/wiki/index.php5?title=16_X_2_LCD_Keypad_Shield_for_Arduino
byte readButtons()
{
   unsigned int buttonVoltage;
   byte button = BUTTON_NONE;   // return no button pressed if the below checks don't write to btn
 
   //read the button ADC pin voltage
   buttonVoltage = analogRead( BUTTON_ADC_PIN );
   //sense if the voltage falls within valid voltage windows
   if( buttonVoltage < ( RIGHT_10BIT_ADC + BUTTONHYSTERESIS ) )
   {
      button = BUTTON_RIGHT;
   }
   else if(   buttonVoltage >= ( UP_10BIT_ADC - BUTTONHYSTERESIS )
           && buttonVoltage <= ( UP_10BIT_ADC + BUTTONHYSTERESIS ) )
   {
      button = BUTTON_UP;
   }
   else if(   buttonVoltage >= ( DOWN_10BIT_ADC - BUTTONHYSTERESIS )
           && buttonVoltage <= ( DOWN_10BIT_ADC + BUTTONHYSTERESIS ) )
   {
      button = BUTTON_DOWN;
   }
   else if(   buttonVoltage >= ( LEFT_10BIT_ADC - BUTTONHYSTERESIS )
           && buttonVoltage <= ( LEFT_10BIT_ADC + BUTTONHYSTERESIS ) )
   {
      button = BUTTON_LEFT;
   }
   else if(   buttonVoltage >= ( SELECT_10BIT_ADC - BUTTONHYSTERESIS )
           && buttonVoltage <= ( SELECT_10BIT_ADC + BUTTONHYSTERESIS ) )
   {
      button = BUTTON_SELECT;
   }
   //handle button flags for just pressed and just released events
   if( ( buttonWas == BUTTON_NONE ) && ( button != BUTTON_NONE ) )
   {
      //the button was just pressed, set buttonJustPressed, this can optionally be used to trigger a once-off action for a button press event
      //it's the duty of the receiver to clear these flags if it wants to detect a new button change event
      buttonJustPressed  = true;
      buttonJustReleased = false;
   }
   if( ( buttonWas != BUTTON_NONE ) && ( button == BUTTON_NONE ) )
   {
      buttonJustPressed  = false;
      buttonJustReleased = true;
   }
 
   //save the latest button value, for change event detection next time round
   buttonWas = button;
 
   return( button );
}


