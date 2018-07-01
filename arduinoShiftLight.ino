// hopefully no global vars
bool ok = true;
bool verbose = true;

// eeprom
#include <EEPROMex.h>
#define CONFIG_VERSION "ls1"
#define MEMORY_BASE 32
int configAddress = 0;

// neopixel
#include <Adafruit_NeoPixel.h>
#include <avr/power.h>
// alpha-quad
#include <Wire.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"
// rotary encoder
#include <Rotary.h>

//SettingsStruct Settings;
struct SettingsStruct {
  char version[4];
  byte brightness;
  unsigned int enable_rpm;
  unsigned int shift_rpm;
  byte color_primary;  // colors will have to be represented as 0xGGRRBB
  byte color_secondary;
  byte color_tertiary;
  byte color_shift_primary;
  byte color_shift_secondary;
  //unsigned int mySettings[9];
} Settings = {
  CONFIG_VERSION,
  0x7F, 0xDAC, 0x1D4C, 0x50, 0x38, 0xC, 0xFC, 0xE0
// 127,  3500,   7500, colors
};

#define NEO_PIN 5
// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_GRB     Pixels are wired for GRB bitstream (mtost NeoPixel products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, NEO_PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

Adafruit_AlphaNum4 alpha4 = Adafruit_AlphaNum4();
char displaybuffer[5] = {' ', ' ', ' ', ' ', ' '};

// rotary encoder
// outputs on pins 0 and 1
Rotary rotary = Rotary(0,1);
int clickCounter = 0;
int clickCounterLast = 0; 

// button on rotary encoder
#define buttonPin 7
// stuff for rotary encoder on interrupts
// http://www.buxtronix.net/2011/10/rotary-encoders-done-properly.html
// https://github.com/buxtronix/arduino/tree/master/libraries/Rotary
#define encoder0PinA 0 // int 2 is pin 0 in micro
#define encoder0PinB 1 // int 3 is pin 1 in micro
/*
 * volatile int encoder0Pos = 0;
 * volatile int lastencoder0Pos = 0;
 * #define readA bitRead(PIND,2)//faster than digitalRead()
 * #define readB bitRead(PIND,3)//faster than digitalRead()
*/

// menu state machine
boolean menuState = false;
boolean lastMenuState = false;
volatile int menuPos = 0;
volatile int lastmenuPos;
boolean setItemState = false;
boolean lastSetItemState = false;
volatile int setItemPos = 0;
boolean rootItemState = false;

//configuration for the Tachometer variables 
#define tachPin 3 
int rpm; 
int rpmlast = 3000; 
int rpm_interval = 3000;
const int timeoutValue = 5; 
volatile unsigned long lastPulseTime; 
volatile unsigned long interval = 0; 
volatile int timeoutCounter;


void setup() {
  Serial.begin(115200);
  
  if ( verbose ) {
    delay(1000);
    Serial.println("hello world");
    delay(1000);
  }
  
  EEPROM.setMemPool(MEMORY_BASE, EEPROMSizeMicro); // set memorypool base to 32, micro board
  configAddress = EEPROM.getAddress(sizeof(SettingsStruct)); // size of config object
  ok = loadConfig();
  alpha4.setBrightness(Settings.brightness / 16);

  // quadalpha setup

  alpha4.begin(0x70);  // pass in the address
  //pinMode(ALPHAPWM, OUTPUT);
  //analogWrite(ALPHAPWM, 255);
  //String("XXXX").toCharArray(displaybuffer,4);
  //alpha4print();
  //for (int i = 0; i < 255; i++ ) {
  //  Serial.println("fadein: " + String(i));
  //  analogWrite(ALPHAPWM, i);
  //  delay(10);
  //}
  alpha4DashChase();
  delay(100);
    
  // neopixel setup
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  rainbow(5);
  //rainbowCycle(5);
  delay(100);
  blackout();
  
  alpha4.clear();
  alpha4.writeDisplay();

  // display every character, 
  //for (uint8_t i='!'; i<='z'; i++) {
  //  alpha4.writeDigitAscii(0, i);
  //  alpha4.writeDigitAscii(1, i+1);
  //  alpha4.writeDigitAscii(2, i+2);
  //  alpha4.writeDigitAscii(3, i+3);
  //  alpha4.writeDisplay();
    
  //  delay(300);
  //}
  
//  String("Tach").toCharArray(displaybuffer,5);
//  alpha4print();
//  delay(10);
  
  // button setup
  attachInterrupt(digitalPinToInterrupt(buttonPin), button, RISING);
  
  // rotary encoder setup
  pinMode(encoder0PinA, INPUT_PULLUP); 
  pinMode(encoder0PinB, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(encoder0PinA), rotate, CHANGE); 
  attachInterrupt(digitalPinToInterrupt(encoder0PinB), rotate, CHANGE); 

  // config for the Tach 
  /*
  pinMode(tachPin, INPUT); 
  // digitalWrite(sensorPin, HIGH); // enable internal pullup (if Hall sensor needs it) 
  attachInterrupt(digitalPinToInterrupt(tachPin), tachIsr, RISING); 
  lastPulseTime = micros(); 
  */
  timeoutCounter = 0; 
}


/*
 * MAIN 
 */
void loop() {  
/*  
 *   if ( verbose ) {
 *   delay(100); // pause to not overload serial io
 * } else {
 *   delay(10);
 * }
 */

  /*
   * 
   * 
   * start of main block
   * 
   * 
   */

  // tachometer routine
  if (timeoutCounter != 0) { 
    --timeoutCounter; 

    //This calculation will need to be calibrated for each application 
    float rpm = 60e6/(float)interval; // ORIGINAL 
    //rpm = 21e6/interval; // CALIBRATED 
    // Serial.print(rpm); 
    // matrix.println(rpm); 
    // matrix.writeDisplay(); 
  } 
  
  //remove erroneous results 
  if (rpm > rpmlast+500){rpm=rpmlast;} 
  rpmlast = rpm; 
  //Let's keep this RPM value under control, between 0 and 8000 
  rpm = constrain (rpm, 0, 8000); 
  // given the nature of the RPM interrupt reader, a zero reading will produce a max result 
  // this fixes this quirk 
  if (rpm==8000){rpm=0;} 
  if ((micros() - lastPulseTime) < 5e6 ) { 
    Serial.println(rpm); 
    String(rpm).toCharArray(displaybuffer,5);
    alpha4print(); 
    delay(10); 
  } 
  else { 
    // if there is no rpm measured we should do something like blank out the display
//    matrix.clear(); 
//    matrix.writeDisplay(); 
  } 


   
  if ( menuState && !setItemState ) { // HIGH LOW
    // scroll through menu items
    if ( clickCounter > clickCounterLast && menuPos < 9 ) {
      ++menuPos;
        clickCounterLast = clickCounter;

    } else if ( clickCounter < clickCounterLast && menuPos > 0 ) {
      --menuPos;
        clickCounterLast = clickCounter;

    }
    if ( verbose ) {
      Serial.print("encoder: " + String(clickCounter) + " last encoder: " + String(clickCounterLast) + " menu: " + String(menuPos));
      Serial.println(" menu: " + String(menuState) + " setItem: " + String(setItemState) + " rootItem: " + String(rootItemState));
    }
  } else if ( menuState && setItemState ) { // HIGH HIGH
    // scroll through menu item settings
    // just realized they need to be tracked independently
    // RPM scale from 0 to 8000
    // ENABLE and SHIFT
    // Color can be from 0x00 00 00 to 0xFF FF FF
    // Wheel() can be 0x00 to 0xFF...
    switch (menuPos) {
      case 1: // brightness and colors
        // maintain absolute range
        // usable range seems to be more like 0x2F is super dim
        // and 0x6B is bright, but not painfully so
        // and of course anything above that is silly
        
        if ( clickCounter > clickCounterLast ) {
          Settings.brightness += 16;
          
        } else if ( clickCounter < clickCounterLast ) {
          Settings.brightness -= 16;
        }
        if ( Settings.brightness < 16 ) { Settings.brightness = 16; }
        if ( Settings.brightness > 240 ) { Settings.brightness = 240; }
        clickCounterLast = clickCounter;

        //analogWrite(ALPHAPWM, mySettings[SHIFTLIGHT_BRIGHTNESS]);
        alpha4.setBrightness(Settings.brightness / 16);
        String(Settings.brightness).toCharArray(displaybuffer,5);
        alpha4print();
        colorFill(Wheel(Settings.color_primary),Settings.brightness);
        break;
      case 2: // enable rpm
        if ( clickCounter > clickCounterLast && Settings.enable_rpm <= Settings.shift_rpm - 200 ) {
          Settings.enable_rpm += 100;
        } else if ( clickCounter < clickCounterLast && Settings.enable_rpm > 2500 ) {
          Settings.enable_rpm -= 100;
        }
          clickCounterLast = clickCounter;
        String(Settings.enable_rpm).toCharArray(displaybuffer,5);
        alpha4print();
        break;
      case 3: // shift rpm
        /*
        if ( encoder0Pos > lastencoder0Pos && mySettings[menuPos] <= 9800) {
          mySettings[menuPos] += 100;
        } else if ( encoder0Pos < lastencoder0Pos && mySettings[menuPos] > mySettings[SHIFTLIGHT_ENABLE_RPM] ) {
          mySettings[menuPos] -= 100;
        }
        */
        if ( clickCounter > clickCounterLast && Settings.shift_rpm <= 9800 ) {
          Settings.shift_rpm += 100;
        } else if ( clickCounter < clickCounterLast && Settings.shift_rpm > Settings.enable_rpm ) {
          Settings.shift_rpm -= 100;
        }
  clickCounterLast = clickCounter;

        // lastencoder0Pos = encoder0Pos;
        // print rpm on quadalpha
        //String(mySettings[menuPos]).toCharArray(displaybuffer,5);
        String(Settings.shift_rpm).toCharArray(displaybuffer,5);
        alpha4print();
        break;
      case 4: // color 1
        if ( clickCounter > clickCounterLast ) {
          Settings.color_primary += 4;
        } else if ( clickCounter < clickCounterLast ) {
          Settings.color_primary -= 4;
        }
        if ( Settings.brightness < 8 ) { Settings.brightness = 252; }
        if ( Settings.brightness > 252 ) { Settings.brightness = 8; }
        // lastencoder0Pos = encoder0Pos
          clickCounterLast = clickCounter;

        String(Settings.color_primary).toCharArray(displaybuffer,5);
        alpha4print();
        colorFill(Wheel(Settings.color_primary),Settings.brightness);
        break;
      case 5: // color 2
        if ( clickCounter > clickCounterLast ) {
          Settings.color_secondary += 4;
        } else if ( clickCounter < clickCounterLast ) {
          Settings.color_secondary -= 4;
        }
        if ( Settings.brightness < 8 ) { Settings.brightness = 252; }
        if ( Settings.brightness > 252 ) { Settings.brightness = 8; }
          clickCounterLast = clickCounter;

        // lastencoder0Pos = encoder0Pos;
        String(Settings.color_secondary).toCharArray(displaybuffer,5);
        alpha4print();
        colorFill(Wheel(Settings.color_secondary),Settings.brightness);
        break;
      case 6: // color 3
        if ( clickCounter > clickCounterLast ) {
          Settings.color_tertiary += 4;
        } else if ( clickCounter < clickCounterLast ) {
          Settings.color_tertiary -= 4;
        }
        if ( Settings.brightness < 8 ) { Settings.brightness = 252; }
        if ( Settings.brightness > 252 ) { Settings.brightness = 8; }
          clickCounterLast = clickCounter;

        // lastencoder0Pos = encoder0Pos;
        String(Settings.color_tertiary).toCharArray(displaybuffer,5);
        alpha4print();
        colorFill(Wheel(Settings.color_tertiary),Settings.brightness);
        break;
      case 7: // shift color 1
        if ( clickCounter > clickCounterLast ) {
          Settings.color_shift_primary += 4;
        } else if ( clickCounter < clickCounterLast ) {
          Settings.color_shift_primary -= 4;
        }
        if ( Settings.brightness < 8 ) { Settings.brightness = 252; }
        if ( Settings.brightness > 252 ) { Settings.brightness = 8; }
          clickCounterLast = clickCounter;

        // lastencoder0Pos = encoder0Pos;
        String(Settings.color_shift_primary).toCharArray(displaybuffer,5);
        alpha4print();
        colorFill(Wheel(Settings.color_shift_primary),Settings.brightness);
        break;
      case 8: // shift color 2
        if ( clickCounter > clickCounterLast ) {
          Settings.color_shift_secondary += 4;
        } else if ( clickCounter < clickCounterLast ) {
          Settings.color_shift_secondary -= 4;
        }
        if ( Settings.brightness < 8 ) { Settings.brightness = 252; }
        if ( Settings.brightness > 252 ) { Settings.brightness = 8; }
          clickCounterLast = clickCounter;

        // lastencoder0Pos = encoder0Pos;
        String(Settings.color_shift_secondary).toCharArray(displaybuffer,5);
        alpha4print();
        colorFill(Wheel(Settings.color_shift_secondary),Settings.brightness);
        break;
      default: // brightness and colors
        clickCounterLast = clickCounter;

        /*
        if ( encoder0Pos > lastencoder0Pos && mySettings[menuPos] < 251) {
          mySettings[menuPos] += 4;
        } else if ( encoder0Pos < lastencoder0Pos && mySettings[menuPos] > 0 ) {
          mySettings[menuPos] -= 4;
        }
        lastencoder0Pos = encoder0Pos;
        colorFill(Wheel(mySettings[menuPos]),mySettings[SHIFTLIGHT_BRIGHTNESS]);
        */
        break;
    }
    
    if ( verbose ) {
      Serial.print("encoder: " + String(clickCounter) + " last encoder: " + String(clickCounterLast) + " menu: " + String(menuPos));
      Serial.print(" menu: " + String(menuState) + " setItem: " + String(setItemState) + " rootItem: " + String(rootItemState));
      Serial.print(" brt: " + String(Settings.brightness,HEX) + 
        " enab: " + String(Settings.enable_rpm,DEC) +
        " shft: " + String(Settings.shift_rpm,DEC) +
        " col1: " + String(Settings.color_primary,HEX));
      Serial.println( " col2: " + String(Settings.color_secondary,HEX) +
        " col3: " + String(Settings.color_tertiary,HEX) +
        " sft1: " + String(Settings.color_shift_primary,HEX) +
        " sft2: " + String(Settings.color_shift_secondary,HEX));
    }
  } else {
//      lastencoder0Pos = encoder0Pos;
      if ( verbose ) {
        Serial.print("encoder: " + String(clickCounter) + " last encoder: " + String(clickCounterLast) + " menu: " + String(menuPos));
        Serial.println(" menu: " + String(menuState) + " setItem: " + String(setItemState) + " rootItem: " + String(rootItemState));
      }
  }

  if ( menuState && !setItemState ) {
    switch (menuPos) {
      case 0:
        rootItemState = true;
        //displaybuffer[0] = 0x39;
        //displaybuffer[1] = 0x9;
        //displaybuffer[2] = 0x9;
        //displaybuffer[3] = 0xF;
        //alpha4printraw();
        //blackout();
        String("EXIT").toCharArray(displaybuffer,5);
        alpha4print();
        break;
      case 1:
        rootItemState = false;
        String("Brt ").toCharArray(displaybuffer,5);
        alpha4print();
        /*
        menustring = String("Brightness");
        menustring.toCharArray(longdisplaybuffer,menustring.length());
        alpha4printscroll();
        */
        break;
      case 2:
        rootItemState = false;
        String("Enbl").toCharArray(displaybuffer,5);
        alpha4print();
        break;
      case 3:
        rootItemState = false;
        String("Shft").toCharArray(displaybuffer,5);
        alpha4print();
        break;
      case 4:
        rootItemState = false;
        String("Col1").toCharArray(displaybuffer,5);
        alpha4print();
        break;
      case 5:
        rootItemState = false;
        String("Col2").toCharArray(displaybuffer,5);
        alpha4print();
        break;
      case 6:
        rootItemState = false;
        String("Col3").toCharArray(displaybuffer,5);
        alpha4print();
        break;
      case 7:
        rootItemState = false;
        String("SC 1").toCharArray(displaybuffer,5);
        alpha4print();
        break;
      case 8:
        rootItemState = false;
        String("SC 2").toCharArray(displaybuffer,5);
        alpha4print();
        break;
      case 9:
        rootItemState = true;
        //alpha4.writeDigitRaw(0, 0x39);
        //alpha4.writeDigitRaw(1, 0x9);
        //alpha4.writeDigitRaw(2, 0x9);
        //alpha4.writeDigitRaw(3, 0xF);
        //alpha4.writeDisplay();
        //blackout();
        String("SAVE").toCharArray(displaybuffer,5);
        alpha4print();
        break;
    }
  }
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}

// flood all pixels in range with brightness
void colorFill(uint32_t c, uint8_t brightness) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
  }
  strip.setBrightness(brightness);
  strip.show();
}

void blackout(void) {
   for(int i=0; i<strip.numPixels(); i++) {
     strip.setPixelColor(i,0);
   }
   strip.show();
}

void rainbow(uint8_t wait) {
  uint16_t i, j;
  strip.setBrightness(32);
  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  //for(j=0; j<256*2; j++) { // 2 cycles of all colors on wheel, was 5
  for(j=0; j<250*2; j++) { // 2 cycles of all colors on wheel, was 5
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
      strip.setBrightness( (j / 5 / 16) + 64);
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();
     
      delay(wait);
     
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
        for (int i=0; i < strip.numPixels(); i=i+3) {
          strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
        }
        strip.show();
       
        delay(wait);
       
        for (int i=0; i < strip.numPixels(); i=i+3) {
          strip.setPixelColor(i+q, 0);        //turn every third pixel off
        }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {

  WheelPos = 255 - WheelPos; // why do we invert WheelPos?
  if (WheelPos < 6 ) {
    return strip.Color(255,255,255);
  } else if(WheelPos > 250) {
    return strip.Color(0,0,0);
  } else if(WheelPos < 85) {
                     // full red,    no green,     blue
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if(WheelPos < 170) {
    WheelPos -= 85;
                     // no red, full green,  blue
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else { // blue
   WheelPos -= 170;
                     // full red, green, no blue
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }

  
  // 4 blocks, 64
  //WheelPos = 255 - WheelPos;
  //if(WheelPos < 64) {
  //  return strip.Color(255 - WheelPos * 4, 0, WheelPos * 4);
  //} else if(WheelPos < 128) {
  //  WheelPos -= 64;
  //  return strip.Color(0, WheelPos * 4, 255 - WheelPos * 4);
  //} else if(WheelPos < 192) {
  //  WheelPos -= 128;
  //  return strip.Color(WheelPos * 4, 255 - WheelPos * 4, 0);
  //} else {
  //  WheelPos -= 192;
  //  return strip.Color(WheelPos * 4, WheelPos * 4, WheelPos * 4);
  //}
}

//
// Quad Alphanumeric Display functions
//

void alpha4print( void ) {
  for (int i = 0; i < 4; i++ ) {
    alpha4.writeDigitAscii(i, displaybuffer[i]);
  }
  alpha4.writeDisplay();
  delay(25);
}

void alpha4printraw( void ) {
  for (int i = 0; i < 4; i++ ) {
    alpha4.writeDigitRaw(i, displaybuffer[i]);
  }
  alpha4.writeDisplay();
  delay(25);
}

/*
void alpha4printscroll( void ) {
  // scroll print the long string
  // finish by leaving the static short string
  // we need to know if we printed the long string or not though
  // and only print the scrolly bit when we need to
  String("     ").toCharArray(displaybuffer,4);
  alpha4print();
  for (int i = 0; i < menustring.length(); i++) {
    displaybuffer[0] = displaybuffer[1];
    displaybuffer[1] = displaybuffer[2];
    displaybuffer[2] = displaybuffer[3];
    displaybuffer[3] = menustring[i];
    alpha4print();
    delay(250);
  }
  delay(500);
}
*/

void alpha4DashChase() {
 // 16-bit digits in each alpha
 // 0 DP N M L K J H G2 G1 F E D C B A
 //
 //     A              -
 // F H J K B      | \ | / |
 //   G1 G2          -   -
 // E L M N C      | / | \ |
 //     D     DP       -     *
 // 
  int buffer[2] = { 0x40, 0x80 };
  
  //Serial.println("clear, then write all dashes");
  
  alpha4.clear();  // clear display first
  alpha4.writeDisplay();
  displaybuffer[0] = 0xC0;
  displaybuffer[1] = 0xC0;
  displaybuffer[2] = 0xC0;
  displaybuffer[3] = 0xC0;
  alpha4print();
  delay(100);
  
  //Serial.println("clear to prep for animated dash chase");
  for (int i=0; i < 4; i++) {
      alpha4.writeDigitRaw(i, 0x00);
  }
  alpha4.writeDisplay();
    
  for (int k = 0; k < 1; k++) {
    alpha4.clear();  // clear display first
    alpha4.writeDisplay();
  
    for (int i=0; i < 4; i++) {
      for (int j=0; j < 2; j++) {
        alpha4.writeDigitRaw(i, buffer[j]);
        alpha4.writeDisplay();
        delay(50);
        //Serial.println("dash chase i: " + String(i) + " j: " + String(j) + " digit: " + String(buffer[j],HEX));
        alpha4.writeDigitRaw(i, 0x00);
      }
    }
    alpha4.clear();  // clear display
    alpha4.writeDisplay();
    for (int i=3; i >= 0; i--) {
      for (int j=1; j >= 0; j--) {
        alpha4.clear();
        alpha4.writeDigitRaw(i, buffer[j]);
        alpha4.writeDisplay();
        delay(50);
        //Serial.println("dash chase i: " + String(i) + " j: " + String(j) + " digit: " + String(buffer[j],HEX));
      }
    }
  }
  
  alpha4.clear();
  alpha4.writeDisplay();
  return;
}

//
// Button function
//

void button()
{
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // If interrupts come faster than 50ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 50) 
  {
    // basically you can either be in the top level menu that lists your parameters
    // or you can be in the menu for one of the parameters
    // state 1: tach mode + button press = shift to state 2 (menu)
    // state 2a: menu + special parameter + button press = shift to state 1 (go back to tach)
    // state 2b: menu + parameter + button press = shift to state 3 (set parameter)
    // state 3: set parameter + button press = save parameter + back to state 2a
    if ( menuState && setItemState && !rootItemState ) { // HIGH HIGH LOW
      // button press here should write to EEPROM
      // then move back to menu item selection
      lastSetItemState = setItemState;
      setItemState = false;
      //if ( ok )
      saveConfig();
    } else if ( menuState && !setItemState && !rootItemState ) { // HIGH LOW LOW
      // we are at a menu item that we want to set
      lastSetItemState = setItemState;
      setItemState = true;
    } else if ( menuState && !setItemState && rootItemState ) { // HIGH LOW HIGH
      // we are at the menu root and need to get out of the menu
      lastMenuState = menuState;
      menuState = false;
    } else {
      lastMenuState = menuState;
      menuState = true;
    }
  }
  last_interrupt_time = interrupt_time;
}

// rotate is called anytime the rotary inputs change state.
void rotate() {
  unsigned char result = rotary.process();
  if (result == DIR_CW) {
    clickCounter--;
  } else if (result == DIR_CCW) {
    clickCounter++;
  }
}

bool loadConfig() {
  if ( verbose ) {
    delay(1000);
    Serial.println("      read settings from EEPROM    ");
  }
  EEPROM.readBlock(configAddress,Settings);
  if ( Settings.enable_rpm > 9000 ) // insurance. if unset this will be max unsigned int
    Settings.enable_rpm = 6500;
  if ( Settings.shift_rpm > 9000 )
    Settings.shift_rpm = 7500;
  if ( verbose ) {
//    Serial.print("encoder: " + String(encoder0Pos) + " last encoder: " + String(lastencoder0Pos) + " menu: " + String(menuPos));
    Serial.print(" menu: " + String(menuPos));
    Serial.print(" menu: " + String(menuState) + " setItem: " + String(setItemState) + " rootItem: " + String(rootItemState));
    Serial.print(" brt: " + String(Settings.brightness,HEX) + 
        " enab: " + String(Settings.enable_rpm,DEC) +
        " shft: " + String(Settings.shift_rpm,DEC) +
        " col1: " + String(Settings.color_primary,HEX));
    Serial.println( " col2: " + String(Settings.color_secondary,HEX) +
        " col3: " + String(Settings.color_tertiary,HEX) +
        " sft1: " + String(Settings.color_shift_primary,HEX) +
        " sft2: " + String(Settings.color_shift_secondary,HEX));
  }
  return (Settings.version == CONFIG_VERSION);
}

void saveConfig() {
  if ( verbose )
    Serial.println("      write settings to EEPROM     ");
  EEPROM.writeBlock(configAddress,Settings);
  if ( verbose ) {
    delay(1000);
    ok = loadConfig();
    delay(1000);
  }
}

void tachIsr() 
{ 
  unsigned long now = micros(); 
  interval = now - lastPulseTime; 
  lastPulseTime = now; 
  timeoutCounter = timeoutValue; 
} 

