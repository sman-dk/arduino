/*
 Copyright 2016, Georg Sluyterman <georg@sman.dk
 License: 2-clause BSD license ("Simplified BSD License" or "FreeBSD License"). 
 Please see the LICENSE file in the parent folder for further information.
*/

/*
 * Ideas for features/changes:
 * - Make the light from the Neopixels warmer (the light seems too violet)
 * - Make it controllable via WiFi (perhaps not that usefull, but... why not! :)
 * - PIR sensor autoenable, not just autodisable
 */

#include <Adafruit_NeoPixel.h>

#define RELAY D1
#define NEOPIXEL D3
#define FANBTN D7
#define LEDBTN D6
#define PIR D2

const unsigned int sleepTime = 10; // For the main loop
const uint8_t numNeopixel = 14;
const unsigned int fanInactivityLimit = 60 * 1000;
const unsigned int ledInactivityLimit = 1800 * 1000 ;
const unsigned int debounceDelay = 50; // simple debounce for buttons
unsigned long long tPIR = 0; // number of sleepTime since last movement in front of the PIR sensor
boolean fanOn = false;
boolean keepFanOn = false; // Auto turn off fan as long as the ledInactivityTimer permits
boolean ledOn = true;
boolean fanButtonInterrupt = false;
boolean ledButtonInterrupt = false;
boolean pirInterrupt = false;
boolean fanToggled = false;
boolean ledToggled = false;
unsigned long long fanButtonPressedTime = 0;
unsigned long long ledButtonPressedTime = 0;
unsigned long long pirHighTime = 0;
unsigned long long deltaTime = 0;
uint8_t brightness = 10;
// simple 11 step 2.2 gamma lookup table.
uint8_t GammaLUT[] = { 0, 1,  7,  18,  33,  55,  82,  116,  156,  202,  255};

Adafruit_NeoPixel strip = Adafruit_NeoPixel(numNeopixel, NEOPIXEL, NEO_GRB + NEO_KHZ800);

void setup() {
  pinMode(RELAY, OUTPUT);
  pinMode(NEOPIXEL, OUTPUT);
  pinMode(FANBTN, INPUT_PULLUP);
  pinMode(LEDBTN, INPUT_PULLUP);
  pinMode(PIR, INPUT);
  digitalWrite(RELAY, LOW);
  //Serial.begin (115200);
  setLED(0);
  attachInterrupt(FANBTN, fan, FALLING);
  attachInterrupt(LEDBTN, led, FALLING);
  attachInterrupt(PIR, pir, RISING);
}

void fan () {
  if ( fanButtonInterrupt == false ) {
    fanButtonInterrupt = true;
    fanToggled = false;
    keepFanOn = false;
    fanButtonPressedTime = millis();
    tPIR = millis();
  }
}

void led (){
  if ( ledButtonInterrupt == false ) {
    ledButtonInterrupt = true;
    ledToggled = false;
    ledButtonPressedTime = millis();
    tPIR = millis();
  }
}

void pir (){
  if ( pirInterrupt == false ) {
    pirInterrupt = true;
    pirHighTime = millis();
  }
}

void setLED (uint8_t intensity) {
  for (uint8_t i = 0; i < numNeopixel; i++) {
    strip.setPixelColor(i, GammaLUT[intensity], GammaLUT[intensity], GammaLUT[intensity]);
  }
  strip.show();
}

// the loop function runs over and over again forever
void loop() {
//  char buffer[20];
//  sprintf(buffer, "%0ld", tPIR);
//  Serial.print(buffer); 
  delay(sleepTime);
  // Fan
  if ( fanButtonInterrupt == true ) {
    if ( digitalRead(FANBTN) == HIGH ) {
      fanButtonInterrupt = false;
    } else {
      deltaTime = millis() - fanButtonPressedTime;
      if ( deltaTime > debounceDelay and deltaTime < 1000 ){
        // toggle fan state
        if ( fanToggled == false ) {
          fanToggled = true;
          if (fanOn) {
            digitalWrite(RELAY, LOW);
            fanOn = false;
          } else {
            digitalWrite(RELAY, HIGH);
            fanOn = true;
          }
        } 
      } else if ( deltaTime >= 1000 ) {
        // Enable keepFanOn
        keepFanOn = true;
        // Indicate keepFanOn to the user
        digitalWrite(RELAY, LOW);
        delay(500);
        digitalWrite(RELAY, HIGH);
        fanButtonInterrupt = false;
      }
    }
  } 
  
  // LED
  if ( ledButtonInterrupt == true ) {
    tPIR = millis();
    if ( digitalRead(LEDBTN) == HIGH ) {
      ledButtonInterrupt = false;
    } else {
      deltaTime = millis() - ledButtonPressedTime;
      if ( deltaTime > debounceDelay and deltaTime < 1000 ){
        // toggle led state
        if (ledToggled == false) {
          ledToggled = true;
          if (ledOn) {
            setLED(0);
            ledOn = false;
            ledButtonInterrupt = false;
          } else {
            setLED(brightness);
            ledOn = true;
          }
        } 
      } else {
        // Dimm the light
        unsigned int incTime = 500;
        for (uint8_t i = 9; i > 0; i--) {
          if ( deltaTime >= 1000 + i*incTime and deltaTime < 1000 + i*incTime + incTime ) {
            brightness = 10-i;
            setLED(brightness);
          }
        }
        // Check if button has been held down through the whole cycle and start all over again if it has
        if(deltaTime >= 1000 + 10*incTime) {
          setLED(0);
          delay(125);
          setLED(5);
          delay(125);
          setLED(brightness);
        }
      }
    }
  } 
  // PIR
  if ( pirInterrupt == true ){
    deltaTime = millis() - pirHighTime;
    if ( deltaTime > debounceDelay ){
      if ( digitalRead(PIR) == HIGH ) {
        tPIR = millis();
      }
      pirInterrupt = false;
    }
  }
  // Fan inactivity autodisable
  if (fanOn) {
    deltaTime = millis() - tPIR;
    if (keepFanOn == true) {
      // use ledInactivityLimit instead of fanInactivityLimit
      if ( deltaTime > ledInactivityLimit) {
        digitalWrite(RELAY, LOW);
        fanOn = false;
      }
    } else { 
      if ( deltaTime > fanInactivityLimit) {
        digitalWrite(RELAY, LOW);
        fanOn = false;
      }
    }
  }
  // LED inactivity autodisable
  if (ledOn && millis() - tPIR > ledInactivityLimit ) {
    setLED(0);
    ledOn = false;
  }
}
