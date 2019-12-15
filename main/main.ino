/*
  betterBatteryBoost:main

  Responsibly controls and monitors a common battery charger + boost circuit,
  and drives an LED strip.
  
  Read more on my blog post here: https://www.andersknelson.com/blog/?p=765
*/

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

  Serial.begin(115200);
  
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
  Serial.println("Button Interrupt!");
}

void intChargeActive(void) {
 
  if(digitalRead(STATUS_CHRG_PIN))
  { 
    //TODO: if pin is HIGH, charge is inactive.
    Serial.println("Not charging.");
  }
  else
  {
    //TODO: if pin is LOW, charge is active. 
    //Display low-power indication, use sleep modes, inhibit application.
    Serial.println("Charging.");
  }
}

void intChargeDone(void) {
  
  if(digitalRead(STATUS_DONE_PIN))
  {
    //TODO: if pin is HIGH, charge is complete.
    //Display low-power indication, use sleep modes, inhibit application.
    Serial.println("Charge complete.");
  }
  else
  {
    //TODO: if pin is LOW, charge is complete.
    //Display low-power indication, use sleep modes, inhibit application.
    Serial.println("Charge complete.");
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
      Serial.println("Pressed");
      if(!stage)
      {
        stage = 1;
        stageChange = 1;
      }
    }
  }
  else
  {
    if(buttonDown)
    {
      buttonDown = 0;
      Serial.println("Released");
    }
  }

  if(stageChange)
  {
    Serial.print("Stage ");
    Serial.println(stage);
    //Serial.print('\n');
    
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
        //while(!SerialUSB);
      default:
  
      break;
    }

    stageChange = 0;
  }

  if(patternActive)
  {
    random16_add_entropy(random());
  
    // Fourth, the most sophisticated: this one sets up a new palette every
    // time through the loop, based on a hue that changes every time.
    // The palette is a gradient from black, to a dark color based on the hue,
    // to a light color based on the hue, to white.
    //
    //   static uint8_t hue = 0;
    //   hue++;
    //   CRGB darkcolor  = CHSV(hue,255,192); // pure hue, three-quarters brightness
    //   CRGB lightcolor = CHSV(hue,128,255); // half 'whitened', full brightness
    //   gPal = CRGBPalette16( CRGB::Black, darkcolor, lightcolor, CRGB::White);
  
  
    Fire2012WithPalette(); // run simulation frame, using palette colors
  }

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
