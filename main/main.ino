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
#define NUM_LEDS    140

#define BRIGHTNESS  255

CRGB leds[NUM_LEDS];

static uint8_t k, hue;

#define BUTTON_PIN 10
bool buttonDown = 0;

#define LOAD_EN_PIN A0
#define BOOST_EN_PIN 16
#define STATUS_DONE_PIN 15
#define STATUS_CHRG_PIN 14

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
  pinMode(STATUS_CHRG_PIN, INPUT_PULLUP); //charging in progress pin, normally high-Z.
  pinMode(STATUS_DONE_PIN, INPUT_PULLUP); //charging done pin, normally  high-Z.
  attachPCINT(digitalPinToPCINT(BUTTON_PIN), intButton, FALLING);
  attachPCINT(digitalPinToPCINT(STATUS_CHRG_PIN), intChargeActive, CHANGE);
  attachPCINT(digitalPinToPCINT(STATUS_DONE_PIN), intChargeDone, CHANGE);
  
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness( BRIGHTNESS );
}

void intButton(void) {
  //wake up from sleep.
  if(DEBUG) SerialUSB.println("Button Interrupt!");
}

void intChargeActive(void) {
 
  if(digitalRead(STATUS_CHRG_PIN))
  { 
    //if pin is HIGH, charge is inactive.
    if(DEBUG) SerialUSB.println("Not charging.");
  }
  else
  {
    //if pin is LOW, charge is active. 
    //TODO: Display low-power indication, use sleep modes, inhibit application.
    if(DEBUG) SerialUSB.println("Charging.");
  }
}

void intChargeDone(void) {
  
  if(!digitalRead(STATUS_DONE_PIN))
  {
    //if pin is LOW, charge is complete.
    //TODO: Display low-power indication, use sleep modes, inhibit application.
    if(DEBUG) SerialUSB.println("Charge complete.");
  }
}

void loop()
{
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

    k=0; //reset fade loop
    
    switch(stage)
    {
      case 1:
        patternActive = 1;
        digitalWrite(BOOST_EN_PIN, HIGH); //enable voltage boost.
        digitalWrite(LOAD_EN_PIN, HIGH); //enable load.
        hue = HUE_RED;
      break;
  
      case 2:
      hue = HUE_GREEN;
      break;

      case 3:
      hue = HUE_AQUA;
      break;

      case 4:
      hue = HUE_YELLOW;
      break;

      case 5:
      hue = HUE_BLUE;
      break;

      case 6:
      hue = HUE_PINK;
      break;
  
      case 0:
        patternActive = 0;
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

  if(patternActive)
  {
    CHSV colorTemp;
    EVERY_N_MILLISECONDS(4)
    {
      if ( k < 250 ) // Check if target has been reached
      {
        colorTemp = CHSV(hue, 255, k);
        fill_solid( leds, NUM_LEDS, colorTemp);
        k++;

        FastLED.show(); // display this frame
      }
      else
      {
        k=0;
        patternActive = 0;
      }
    }
  }
}
