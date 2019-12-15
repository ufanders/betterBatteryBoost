/*
  betterBatteryBoost:main

  Responsibly controls and monitors a common battery charger + boost circuit,
  and drives an LED strip.
  
  Read more on my blog post here: https://www.andersknelson.com/blog/?p=765
*/
#define DEBUG 0

#include <FastLED.h>

#define LED_PIN     2
#define COLOR_ORDER GRB
#define CHIPSET     WS2812B
#define NUM_LEDS    258

#define BRIGHTNESS  255
#define FRAMES_PER_SECOND 60

bool gReverseDirection = false;

CRGB leds[NUM_LEDS];
CRGBPalette16 gPal;

CHSV colorStart = CHSV(HUE_RED,255,0);  // starting color: black
CHSV colorTarget = CHSV(HUE_RED,255,255);  // target color: red
CHSV colorCurrent = colorStart;
static uint8_t k, hue;

#include <ButtonDebounce.h>

#define BUTTON_PIN 10
//ButtonDebounce button(BUTTON_PIN, 50); //50ms debounce.
bool buttonDown = 0;

#define LOAD_EN_PIN A0
#define BOOST_EN_PIN 16
#define STATUS_DONE_PIN 15
#define STATUS_CHRG_PIN 14

#include <timer.h>

Timer<> timer;
bool patternActive = 0;
bool patternFadeout = 0;
int stage = 0;
bool stageChange = 0;

#include "PinChangeInterrupt.h"
/*
The following pins are usable for PinChangeInterrupt:
 Arduino Uno/Nano/Mini: All pins are usable
 Arduino Mega: 10, 11, 12, 13, 50, 51, 52, 53, A8 (62), A9 (63), A10 (64),
               A11 (65), A12 (66), A13 (67), A14 (68), A15 (69)
 Arduino Leonardo/Micro: 8, 9, 10, 11, 14 (MISO), 15 (SCK), 16 (MOSI)
 HoodLoader2: All (broken out 1-7) pins are usable
 Attiny 24/44/84: All pins are usable
 Attiny 25/45/85: All pins are usable
 Attiny 13: All pins are usable
 Attiny 441/841: All pins are usable
 ATmega644P/ATmega1284P: All pins are usable
*/

#include "LowPower.h"

void setup() {
  delay(2000); //sanity delay

  if(DEBUG) SerialUSB.begin(115200);
  
  digitalWrite(BOOST_EN_PIN, LOW);
  pinMode(BOOST_EN_PIN, OUTPUT); //boost enable pin, normally low.
  digitalWrite(LOAD_EN_PIN, LOW);
  pinMode(LOAD_EN_PIN, OUTPUT); //load enable pin, normally low.

  //These pins use PCINT to wake up from sleep, and must be in PORTB.
  pinMode(BUTTON_PIN, INPUT_PULLUP); //button pin, normally open.
  pinMode(STATUS_CHRG_PIN, INPUT_PULLUP); //chargins in progress pin, normally high-Z.
  pinMode(STATUS_DONE_PIN, INPUT_PULLUP); //charging done pin, normally  high-Z.
  attachPCINT(digitalPinToPCINT(BUTTON_PIN), intButton, FALLING);
  attachPCINT(digitalPinToPCINT(STATUS_CHRG_PIN), intChargeActive, CHANGE);
  attachPCINT(digitalPinToPCINT(STATUS_DONE_PIN), intChargeDone, CHANGE);
  
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness( BRIGHTNESS );

  gPal = HeatColors_p;
  
  // Second, this palette is like the heat colors, but blue/aqua instead of red/yellow
  //gPal = CRGBPalette16( CRGB::Black, CRGB::Blue, CRGB::Aqua,  CRGB::White);

}

void intButton(void) {
  //TODO: wake up from sleep.
  if(DEBUG) SerialUSB.println("Button Interrupt!");
}

void intChargeActive(void) {
 
  if(digitalRead(STATUS_CHRG_PIN))
  { 
    //TODO: if pin is HIGH, charge is inactive.
    if(DEBUG) SerialUSB.println("Not charging.");
  }
  else
  {
    //TODO: if pin is LOW, charge is active. 
    //Display low-power indication, use sleep modes, inhibit application.
    if(DEBUG) SerialUSB.println("Charging.");
  }
}

void intChargeDone(void) {
  
  if(digitalRead(STATUS_DONE_PIN))
  {
    //TODO: if pin is HIGH, charge is complete.
    //Display low-power indication, use sleep modes, inhibit application.
    if(DEBUG) SerialUSB.println("Charge complete.");
  }
  else
  {
    //TODO: if pin is LOW, charge is complete.
    //Display low-power indication, use sleep modes, inhibit application.
    if(DEBUG) SerialUSB.println("Charge complete.");
  }
}

void loop()
{
  //button.update();
  timer.tick();

  //if(button.state() == LOW) //button was pressed
  if(!digitalRead(BUTTON_PIN))
  {
    if(!buttonDown)
    {
      buttonDown = 1;
      if(DEBUG) SerialUSB.println("Pressed");
      stage++;
      if(stage == 7) stage = 0;
      stageChange = 1;
    }
  }
  else
  {
    if(buttonDown)
    {
      buttonDown = 0;
      if(DEBUG) SerialUSB.println("Released");
    }
  }

  if(stageChange)
  {
    if(DEBUG) SerialUSB.print("Stage ");
    if(DEBUG) SerialUSB.println(stage);

    if(k > 0)
    {
      k=0;
      colorCurrent = colorTarget;
    }
    
    switch(stage)
    {
      case 1:
        patternActive = 1;
        digitalWrite(BOOST_EN_PIN, HIGH); //enable voltage boost.
        digitalWrite(LOAD_EN_PIN, HIGH); //enable load.
        colorTarget = CHSV(HUE_RED,255,255);
        hue = HUE_RED;
      break;
  
      case 2:
      patternActive = 1;
      colorTarget = CHSV(HUE_GREEN,255,255);  // target color: green
      hue = HUE_GREEN;
      break;

      case 3:
      patternActive = 1;
      colorTarget = CHSV(HUE_AQUA,255,255);  // target color: green
      hue = HUE_AQUA;
      break;

      case 4:
      patternActive = 1;
      colorTarget = CHSV(HUE_YELLOW,255,255);  // target color: green
      hue = HUE_YELLOW;
      break;

      case 5:
      patternActive = 1;
      colorTarget = CHSV(HUE_BLUE,255,255);  // target color: green
      hue = HUE_BLUE;
      break;

      case 6:
      patternActive = 1;
      colorTarget = CHSV(HUE_PINK,255,255);  // target color: green
      hue = HUE_PINK;
      break;
  
      case 0:
        patternActive = 0;
        colorCurrent = CHSV(HUE_RED,255,0);
        digitalWrite(BOOST_EN_PIN, LOW); //disable voltage boost.
        digitalWrite(LOAD_EN_PIN, LOW); //disable load.

        //NOTE: DO NOT UNPLUG USB UNLESS LEDS ARE OFF.
        //If you do, the program will hang.
        if(DEBUG) SerialUSB.println("Powering down.");
        //SerialUSB.flush(); //does not work.
        //SerialUSB.end();
        //USBDevice.detach();
        
        LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
        //LowPower.idle(SLEEP_FOREVER, ADC_OFF, TIMER4_OFF, TIMER3_OFF, TIMER1_OFF, \
        TIMER0_OFF, SPI_OFF, USART1_OFF, TWI_OFF, USB_ON);

        //if(SerialUSB) USBDevice.attach();
        //SerialUSB.begin(115200);
        if(DEBUG) SerialUSB.println("Powering up.");
        
      default:
      break;
    }

    stageChange = 0;
  }

/*
  if(stageChange)
  {
    SerialUSB.print("Stage ");
    SerialUSB.println(stage);
    //SerialUSB.print('\n');
    
    switch(stage)
    {
      case 1:
        patternActive = 1;
        timer.in(3000, patternFadeoutFcn);
        digitalWrite(BOOST_EN_PIN, HIGH); //enable voltage boost.
        digitalWrite(LOAD_EN_PIN, HIGH); //enable load.
      break;
  
      case 2:
        patternActive = 0;
        patternFadeout = 1;
        timer.in(3000, patternClearFcn);
      break;
  
      case 0:
        patternActive = 0;
        patternFadeout = 0;
        digitalWrite(BOOST_EN_PIN, LOW); //disable voltage boost.
        digitalWrite(LOAD_EN_PIN, LOW); //disable load.
        //USBDevice.detach();
        //LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
        //USBDevice.attach(); //reattach to USB host.
        //while(!SerialUSBUSB);
      default:
  
      break;
    }

    stageChange = 0;
  }
  */

  if(patternActive)
  {
    /*
    random16_add_entropy(random());
    Fire2012WithPalette(); // run simulation frame, using palette colors
    */
    CHSV colorTemp;
    EVERY_N_MILLISECONDS(4)
    {
      if ( k < 250 ) // Check if target has been reached
      {  
        //colorTemp = blend(colorCurrent, colorTarget, k, FORWARD_HUES);
        colorTemp = CHSV(hue, 255, k);
        fill_solid( leds, NUM_LEDS, colorTemp);
        k++;

        FastLED.show(); // display this frame
      }
      else
      {
        k=0;
        //colorCurrent = colorTarget;
        patternActive = 0;
      }
    }
  }
  
  /*
  if(patternFadeout)
  {
    random16_add_entropy(random());
    Fire2012WithPaletteFadeout(); // run simulation frame, using palette colors
  }
  
  if(patternActive || patternFadeout) //while any pattern stage timer is active.
  {
    FastLED.show(); // display this frame
    FastLED.delay(1000 / FRAMES_PER_SECOND);
  }
  */
}


// Less cooling = taller flames.  More cooling = shorter flames.
// Default 55, suggested range 20-100 
#define COOLING  80

// More sparking = more roaring fire.  Less sparking = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 75

byte heat[NUM_LEDS];

void Fire2012WithPalette()
{
// Array of temperature readings at each simulation cell

  // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= NUM_LEDS - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < SPARKING ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < NUM_LEDS/2; j++) {
      // Scale the heat value from 0-255 down to 0-240
      // for best results with color palettes.
      byte colorindex = scale8( heat[j], 240);
      CRGB color = ColorFromPalette( gPal, colorindex);
      int pixelnumber;
      if( gReverseDirection ) {
        pixelnumber = (NUM_LEDS-1) - j;
      } else {
        pixelnumber = j;
      }
      leds[pixelnumber + NUM_LEDS/2] = color; //start at middle of string.
      leds[NUM_LEDS/2 - pixelnumber] = color; //reverse and copy.
    }
}

bool patternFadeoutFcn(void *)
{ 
  stage++;
  stageChange = 1;
  return false; //no repeat.
}

void Fire2012WithPaletteFadeout()
{

  int cooling = 255;
  
    // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((cooling * 10) / NUM_LEDS) + 2));
    }
    
  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for( int k= NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
  }

  // Step 4.  Map from heat cells to LED colors
  for( int j = 0; j < NUM_LEDS/2; j++) {
    // Scale the heat value from 0-255 down to 0-240
    // for best results with color palettes.
    byte colorindex = scale8( heat[j], 240);
    CRGB color = ColorFromPalette( gPal, colorindex);
    int pixelnumber;
    if( gReverseDirection ) {
      pixelnumber = (NUM_LEDS-1) - j;
    } else {
      pixelnumber = j;
    }
    leds[pixelnumber + NUM_LEDS/2] = color; //start at middle of string.
    leds[NUM_LEDS/2 - pixelnumber] = color; //reverse and copy.
  }

}

bool patternClearFcn(void *)
{ 
  patternFadeout = 0;
  stage = 0;
  stageChange = 1;

  FastLED.clear();
  FastLED.show();

  //reset fire values.
  for( int i = 0; i < NUM_LEDS; i++)
  {
      heat[i] = 0;
  }

  return false; //no repeat.
}
