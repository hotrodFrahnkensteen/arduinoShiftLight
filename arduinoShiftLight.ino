#include <EEPROMex.h>
#define CONFIG_VERSION "ls1"
#define MEMORY_BASE 32
bool ok = true;
bool verbose = false;
int configAddress = 0;

#include <Adafruit_NeoPixel.h>
#include <avr/power.h>

#include <Wire.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

// these should all be unsigned int
#define SHIFTLIGHT_ID 0
#define SHIFTLIGHT_BRIGHTNESS 1
#define SHIFTLIGHT_ENABLE_RPM 2
#define SHIFTLIGHT_SHIFT_RPM 3
#define SHIFTLIGHT_COLOR_PRIMARY 4 // colors will have to be represented as 0xGGRRBB
#define SHIFTLIGHT_COLOR_SECONDARY 5
#define SHIFTLIGHT_COLOR_TERTIARY 6
#define SHIFTLIGHT_COLOR_SHIFT_PRIMARY 7
#define SHIFTLIGHT_COLOR_SHIFT_SECONDARY 8

//unsigned int mySettings[9]; // in setup set initial values
struct SettingsStruct {
  char version[4];
  byte brightness;
  unsigned int enable_rpm;
  unsigned int shift_rpm;
  byte color_primary;
  byte color_secondary;
  byte color_tertiary;
  byte color_shift_primary;
  byte color_shift_secondary;
  //unsigned int mySettings[9];
} Settings = {
  CONFIG_VERSION,
  0x7F, 0xDAC, 0x1D4C, 0x50, 0x38, 0xC, 0xFC, 0xE0
};
//SettingsStruct Settings;

#define NEO_PIN 5

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(8, NEO_PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

Adafruit_AlphaNum4 alpha4 = Adafruit_AlphaNum4();
//#define ALPHAPWM 6
char displaybuffer[5] = {' ', ' ', ' ', ' ', ' '};
/*
char longdisplaybuffer[16];
String menustring;
*/

// stuff for rotary encoder on interrupts
#define encoder0PinA 0
#define encoder0PinB 1
volatile int encoder0Pos = 0;
volatile int lastencoder0Pos = 0;
boolean menuState = false;
boolean lastMenuState = false;
volatile int menuPos = 0;
volatile int lastmenuPos;

boolean setItemState = false;
boolean lastSetItemState = false;
volatile int setItemPos = 0;

boolean rootItemState = false;

// button debounce code
int lastButtonState = LOW;   // the previous reading from the input pin

// the following variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 50;    // the debounce time; increase if the output flickers


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
  
  // need to read settings from EEPROM 
  //mySettings[SHIFTLIGHT_ID] = 0; // this should be some static value that can be verified by reading eeprom
  //mySettings[SHIFTLIGHT_BRIGHTNESS] = 127;
  //mySettings[SHIFTLIGHT_ENABLE_RPM] = 3000;
  //mySettings[SHIFTLIGHT_SHIFT_RPM] = 7000;
  //mySettings[SHIFTLIGHT_COLOR_PRIMARY] = 0x50;
  //mySettings[SHIFTLIGHT_COLOR_SECONDARY] = 0x38;
  //mySettings[SHIFTLIGHT_COLOR_TERTIARY] = 0xC;
  //mySettings[SHIFTLIGHT_COLOR_SHIFT_PRIMARY] = 0xFC;
  //mySettings[SHIFTLIGHT_COLOR_SHIFT_SECONDARY] = 0xE0;

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
  
  // neopixel setup
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  rainbowCycle(5);
  delay(400);
  blackout();
  //colorWipe(strip.Color(255, 0, 0), 50); // Red
  //colorWipe(strip.Color(0, 255, 0), 50); // Green
  //colorWipe(strip.Color(0, 0, 255), 50); // Blue
  //colorWipe(strip.Color(0, 0, 0), 50); // Black out
  
  alpha4.writeDigitRaw(3, 0x0);
  alpha4.writeDigitRaw(0, 0xFFFF);
  alpha4.writeDisplay();
  delay(200);
  alpha4.writeDigitRaw(0, 0x0);
  alpha4.writeDigitRaw(1, 0xFFFF);
  alpha4.writeDisplay();
  delay(200);
  alpha4.writeDigitRaw(1, 0x0);
  alpha4.writeDigitRaw(2, 0xFFFF);
  alpha4.writeDisplay();
  delay(200);
  alpha4.writeDigitRaw(2, 0x0);
  alpha4.writeDigitRaw(3, 0xFFFF);
  alpha4.writeDisplay();
  delay(200);
  
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
  String("Tach").toCharArray(displaybuffer,5);
  alpha4print();
  //alpha4.writeDigitAscii(0, 'S');
  //alpha4.writeDigitAscii(1, 't');
  //alpha4.writeDigitAscii(2, 'r');
  //alpha4.writeDigitAscii(3, 't');
  //alpha4.writeDisplay();
  
  // button setup
  attachInterrupt(4, button, RISING);
  
  // rotary encoder setup
  pinMode(encoder0PinA, INPUT); 
  pinMode(encoder0PinB, INPUT);
  attachInterrupt(2, doEncoderA, CHANGE); // int 2 is pin 0 in micro
  attachInterrupt(3, doEncoderB, CHANGE); // int 3 is pin 1 in micro
}

void loop() {  
  if ( verbose ) {
    delay(100); // pause to not overload serial io
  } else {
    delay(10);
  }
  
  //lastencoder0Pos = encoder0Pos;
  if ( menuState && !setItemState ) { // HIGH LOW
    // scroll through menu items
    if ( encoder0Pos > lastencoder0Pos + 3 && menuPos < 9 ) {
      ++menuPos;
      lastencoder0Pos = encoder0Pos;
    } else if ( encoder0Pos < lastencoder0Pos - 3 && menuPos > 0 ) {
      --menuPos;
      lastencoder0Pos = encoder0Pos;
    } else {
      lastencoder0Pos = encoder0Pos;
    }
    if ( verbose ) {
      Serial.print("encoder: " + String(encoder0Pos) + " last encoder: " + String(lastencoder0Pos) + " menu: " + String(menuPos));
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
        /*
        if ( mySettings[SHIFTLIGHT_BRIGHTNESS] > 256 ) {
          mySettings[SHIFTLIGHT_BRIGHTNESS] = 255;
        } else if (mySettings[SHIFTLIGHT_BRIGHTNESS] < 0 ) {
          mySettings[SHIFTLIGHT_BRIGHTNESS] = 0;
        }
        */
        /*
        if ( encoder0Pos > lastencoder0Pos && mySettings[SHIFTLIGHT_BRIGHTNESS] < 251) {
          mySettings[SHIFTLIGHT_BRIGHTNESS] += 4;
        } else if ( encoder0Pos < lastencoder0Pos && mySettings[SHIFTLIGHT_BRIGHTNESS] > 0 ) {
          mySettings[SHIFTLIGHT_BRIGHTNESS] -= 4;
        }
        */
        
        if ( encoder0Pos > lastencoder0Pos && Settings.brightness < 251 ) {
          Settings.brightness += 4;
        } else if ( encoder0Pos < lastencoder0Pos && Settings.brightness > 8 ) {
          Settings.brightness -= 4;
        }
        lastencoder0Pos = encoder0Pos;
        //analogWrite(ALPHAPWM, mySettings[SHIFTLIGHT_BRIGHTNESS]);
        /*
        colorFill(Wheel(mySettings[SHIFTLIGHT_COLOR_PRIMARY]),mySettings[SHIFTLIGHT_BRIGHTNESS]);
        */
        colorFill(Wheel(Settings.color_primary),Settings.brightness);
        break;
      case 2: // enable rpm
        /*
        if ( encoder0Pos > lastencoder0Pos && mySettings[menuPos] <= mySettings[SHIFTLIGHT_SHIFT_RPM] - 200 ) {
          mySettings[menuPos] += 100;
        } else if ( encoder0Pos < lastencoder0Pos && mySettings[menuPos] > 3000 ) {
          mySettings[menuPos] -= 100;
        }
        */
        if ( encoder0Pos > lastencoder0Pos && Settings.enable_rpm <= Settings.shift_rpm - 200 ) {
          Settings.enable_rpm += 100;
        } else if ( encoder0Pos < lastencoder0Pos && Settings.enable_rpm > 3000 ) {
          Settings.enable_rpm -= 100;
        }
        lastencoder0Pos = encoder0Pos;
        //String(mySettings[menuPos]).toCharArray(displaybuffer,5);
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
        if ( encoder0Pos > lastencoder0Pos && Settings.shift_rpm <= 9800 ) {
          Settings.shift_rpm += 100;
        } else if ( encoder0Pos < lastencoder0Pos && Settings.shift_rpm > Settings.enable_rpm ) {
          Settings.shift_rpm -= 100;
        }
        lastencoder0Pos = encoder0Pos;
        // print rpm on quadalpha
        //String(mySettings[menuPos]).toCharArray(displaybuffer,5);
        String(Settings.shift_rpm).toCharArray(displaybuffer,5);
        alpha4print();
        break;
      case 4: // color 1
        if ( encoder0Pos > lastencoder0Pos && Settings.color_primary < 251 ) {
          Settings.color_primary += 4;
        } else if ( encoder0Pos < lastencoder0Pos && Settings.color_primary > 0 ) {
          Settings.color_primary -= 4;
        }
        lastencoder0Pos = encoder0Pos;
        colorFill(Wheel(Settings.color_primary),Settings.brightness);
        break;
      case 5: // color 2
        if ( encoder0Pos > lastencoder0Pos && Settings.color_secondary < 251 ) {
          Settings.color_secondary += 4;
        } else if ( encoder0Pos < lastencoder0Pos && Settings.color_secondary > 4 ) {
          Settings.color_secondary -= 4;
        }
        lastencoder0Pos = encoder0Pos;
        colorFill(Wheel(Settings.color_secondary),Settings.brightness);
        break;
      case 6: // color 3
        if ( encoder0Pos > lastencoder0Pos && Settings.color_tertiary < 251 ) {
          Settings.color_tertiary += 4;
        } else if ( encoder0Pos < lastencoder0Pos && Settings.color_tertiary > 4 ) {
          Settings.color_tertiary -= 4;
        }
        lastencoder0Pos = encoder0Pos;
        colorFill(Wheel(Settings.color_tertiary),Settings.brightness);
        break;
      case 7: // shift color 1
        if ( encoder0Pos > lastencoder0Pos && Settings.color_shift_primary < 251 ) {
          Settings.color_shift_primary += 4;
        } else if ( encoder0Pos < lastencoder0Pos && Settings.color_shift_primary > 4 ) {
          Settings.color_shift_primary -= 4;
        }
        lastencoder0Pos = encoder0Pos;
        colorFill(Wheel(Settings.color_shift_primary),Settings.brightness);
        break;
      case 8: // shift color 2
        if ( encoder0Pos > lastencoder0Pos && Settings.color_shift_secondary < 251 ) {
          Settings.color_shift_secondary += 4;
        } else if ( encoder0Pos < lastencoder0Pos && Settings.color_shift_secondary > 4 ) {
          Settings.color_shift_secondary -= 4;
        }
        lastencoder0Pos = encoder0Pos;
        colorFill(Wheel(Settings.color_shift_secondary),Settings.brightness);
        break;
      default: // brightness and colors
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
      Serial.print("encoder: " + String(encoder0Pos) + " last encoder: " + String(lastencoder0Pos) + " menu: " + String(menuPos));
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
      lastencoder0Pos = encoder0Pos;
      if ( verbose ) {
        Serial.print("encoder: " + String(encoder0Pos) + " last encoder: " + String(lastencoder0Pos) + " menu: " + String(menuPos));
        Serial.println(" menu: " + String(menuState) + " setItem: " + String(setItemState) + " rootItem: " + String(rootItemState));
      }
  }
  if ( menuState && !setItemState ) {
    switch (menuPos) {
      case 0:
        rootItemState = true;
        displaybuffer[0] = 0x39;
        displaybuffer[1] = 0x9;
        displaybuffer[2] = 0x9;
        displaybuffer[3] = 0xF;
        alpha4printraw();
        blackout();
        break;
      case 1:
        rootItemState = false;
        String("Brt ").toCharArray(displaybuffer,5);
        alpha4print();
        /*
        alpha4.writeDigitAscii(0, 'B');
        alpha4.writeDigitAscii(1, 'r');
        alpha4.writeDigitAscii(2, 't');
        alpha4.writeDigitAscii(3, ' ');
        alpha4.writeDisplay();
        */
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
        /*
        alpha4.writeDigitAscii(0, 'E');
        alpha4.writeDigitAscii(1, 'n');
        alpha4.writeDigitAscii(2, 'b');
        alpha4.writeDigitAscii(3, 'l');
        alpha4.writeDisplay();
        */
        break;
      case 3:
        rootItemState = false;
        String("Shft").toCharArray(displaybuffer,5);
        alpha4print();
        /*
        alpha4.writeDigitAscii(0, 'S');
        alpha4.writeDigitAscii(1, 'h');
        alpha4.writeDigitAscii(2, 'f');
        alpha4.writeDigitAscii(3, 't');
        alpha4.writeDisplay();
        */
        break;
      case 4:
        rootItemState = false;
        String("Col1").toCharArray(displaybuffer,5);
        alpha4print();
        /*
        alpha4.writeDigitAscii(0, 'C');
        alpha4.writeDigitAscii(1, 'o');
        alpha4.writeDigitAscii(2, 'l');
        alpha4.writeDigitAscii(3, '1');
        alpha4.writeDisplay();
        */
        break;
      case 5:
        rootItemState = false;
        String("Col2").toCharArray(displaybuffer,5);
        alpha4print();
        /*
        alpha4.writeDigitAscii(0, 'C');
        alpha4.writeDigitAscii(1, 'o');
        alpha4.writeDigitAscii(2, 'l');
        alpha4.writeDigitAscii(3, '2');
        alpha4.writeDisplay();
        */
        break;
      case 6:
        rootItemState = false;
        String("Col3").toCharArray(displaybuffer,5);
        alpha4print();
        /*
        alpha4.writeDigitAscii(0, 'C');
        alpha4.writeDigitAscii(1, 'o');
        alpha4.writeDigitAscii(2, 'l');
        alpha4.writeDigitAscii(3, '3');
        alpha4.writeDisplay();
        */
        break;
      case 7:
        rootItemState = false;
        String("SC 1").toCharArray(displaybuffer,5);
        alpha4print();
        /*
        alpha4.writeDigitAscii(0, 'S');
        alpha4.writeDigitAscii(1, 'C');
        alpha4.writeDigitAscii(2, ' ');
        alpha4.writeDigitAscii(3, '1');
        alpha4.writeDisplay();
        */
        break;
      case 8:
        rootItemState = false;
        String("SC 2").toCharArray(displaybuffer,5);
        alpha4print();
        /*
        alpha4.writeDigitAscii(0, 'S');
        alpha4.writeDigitAscii(1, 'C');
        alpha4.writeDigitAscii(2, ' ');
        alpha4.writeDigitAscii(3, '2');
        alpha4.writeDisplay();
        */
        break;
      case 9:
        rootItemState = true;
        alpha4.writeDigitRaw(0, 0x39);
        alpha4.writeDigitRaw(1, 0x9);
        alpha4.writeDigitRaw(2, 0x9);
        alpha4.writeDigitRaw(3, 0xF);
        alpha4.writeDisplay();
        blackout();
        break;
    }
  }
  //else {
  //    alpha4.writeDigitAscii(0, 'S');
  //    alpha4.writeDigitAscii(1, 't');
  //    alpha4.writeDigitAscii(2, 'r');
  //    alpha4.writeDigitAscii(3, 't');
  //    alpha4.writeDisplay();
  //}

  // Some example procedures showing how to display to the pixels:
  //colorWipe(strip.Color(255, 0, 0), 50); // Red
  //colorWipe(strip.Color(0, 255, 0), 50); // Green
  //colorWipe(strip.Color(0, 0, 255), 50); // Blue
  // Send a theater pixel chase in...
  //theaterChase(strip.Color(127, 127, 127), 50); // White
  //theaterChase(strip.Color(127,   0,   0), 50); // Red
  //theaterChase(strip.Color(  0,   0, 127), 50); // Blue

  //rainbow(20);
  //rainbowCycle(20);
  //theaterChaseRainbow(50);
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
    
  for (int k = 0; k < 5; k++) {
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
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 200) 
  {
   
    // menuState HIGH or LOW
    // setItemState HIGH or LOW
    // rootItemState HIGH or LOW
    // menuState HIGH setItemState LOW rootItemState LOW
    // menuState HIGH setItemState HIGH rootItemState LOW
    // menuState HIGH setItemState LOW rootItemState HIGH
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
  //delay(10); // slight delay for debounce
}

//
// Encoder functions
//

void doEncoderA(){

  // look for a low-to-high on channel A
  if (digitalRead(encoder0PinA) == HIGH) { 
    // check channel B to see which way encoder is turning
    if (digitalRead(encoder0PinB) == LOW) {  
      encoder0Pos = encoder0Pos + 1;         // CW
    } 
    else {
      encoder0Pos = encoder0Pos - 1;         // CCW
    }
  }
  else   // must be a high-to-low edge on channel A                                       
  { 
    // check channel B to see which way encoder is turning  
    if (digitalRead(encoder0PinB) == HIGH) {   
      encoder0Pos = encoder0Pos + 1;          // CW
    } 
    else {
      encoder0Pos = encoder0Pos - 1;          // CCW
    }
  }
  //Serial.println (encoder0Pos, DEC);          
  // use for debugging - remember to comment out
}

void doEncoderB(){

  // look for a low-to-high on channel B
  if (digitalRead(encoder0PinB) == HIGH) {   
   // check channel A to see which way encoder is turning
    if (digitalRead(encoder0PinA) == HIGH) {  
      encoder0Pos = encoder0Pos + 1;         // CW
    } 
    else {
      encoder0Pos = encoder0Pos - 1;         // CCW
    }
  }
  // Look for a high-to-low on channel B
  else { 
    // check channel B to see which way encoder is turning  
    if (digitalRead(encoder0PinA) == LOW) {   
      encoder0Pos = encoder0Pos + 1;          // CW
    } 
    else {
      encoder0Pos = encoder0Pos - 1;          // CCW
    }
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
    Serial.print("encoder: " + String(encoder0Pos) + " last encoder: " + String(lastencoder0Pos) + " menu: " + String(menuPos));
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

