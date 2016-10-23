/*
 Copyright 2016-10-23 Georg Sluyterman <georg@sman.dk
*/
#include <Adafruit_NeoPixel.h>

#define RELAY D1
#define NEOPIXEL D3
#define FANBTN D7
#define LEDBTN D8
#define PIR D4

const uint8_t numNeopixel = 14;
const unsigned int fanInactivityLimit = 3; // FIXME 30
const unsigned int ledInactivityLimit = 5; // FIXME 1800
const unsigned int debounceDelay = 50; // simple debounce, waiting in ms to confirm
unsigned int tPIR = 0; // number os seconds since last movement in front of the PIR sensor
boolean fanOn = false;
boolean keepFanOn = false; // Turn of as long as the ledInactivityTimer permits
boolean ledOn = false;
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
  setLED(10);
  attachInterrupt(FANBTN, fan, RISING);
  attachInterrupt(LEDBTN, led, RISING);
  attachInterrupt(PIR, pir, RISING);
}

void fan () {
  delay(debounceDelay);
  if ( digitalRead(FANBTN) == HIGH ) {
    if (fanOn) {
      digitalWrite(RELAY, LOW);
      keepFanOn = false;
    } else {
      digitalWrite(RELAY, HIGH);
      // If the button continues to held down
      unsigned int fanBtnHolding = 0;
      while ( fanBtnHolding < 1000 ) {
        delay(50);
        if ( digitalRead(FANBTN) == LOW ){
          return; // The button was not held down long enough
        }
      }
      // The button was held down long enough
      digitalWrite(RELAY, LOW); // Indicate to the user
      delay(500);
      digitalWrite(RELAY, HIGH);
      keepFanOn = true;
    }
  }
}

void led (){
  delay(debounceDelay);
  if ( digitalRead(LEDBTN) == LOW ) {
    if (ledOn) {
      setLED(0);
      ledOn = false;
    } else {
      setLED(brightness);
      // If the button continues to held down
      unsigned int fanBtnHolding = 0;
      while ( true) {
        delay(500);
        if ( digitalRead(LEDBTN) == HIGH ){
          return; // The button has been released
        }
        brightness--;
        // We hit the lowest point
        if (brightness == 1) {
          for ( uint8_t i = 0; i < 4; i++){
            if ( digitalRead(LEDBTN) == HIGH ){
               // The button has been released
               setLED(brightness);
               return;
             }
             setLED(10);
             delay(500);
             if ( digitalRead(LEDBTN) == HIGH ){
               // The button has been released
               setLED(brightness);
               return;
             }
             setLED(0);
          } 
        }
        brightness = 10;
      }
      // brightness is now at its lowest
      digitalWrite(RELAY, LOW); // Indicate to the user
      delay(500);
      digitalWrite(RELAY, HIGH);
      keepFanOn = true;
    }
  }
}

void pir (){
  delay(debounceDelay);
  if ( digitalRead(PIR) == HIGH ) {
    tPIR = 0;
    return;
  }
}
void setLED (char intensity) {
  for (uint8_t i = 0; i < numNeopixel; i++) {
    strip.setPixelColor(i, GammaLUT[intensity], GammaLUT[intensity], GammaLUT[intensity]);
  }
  strip.show();
}

// the loop function runs over and over again forever
void loop() {
  // Indicate movement e.g. for debugging
  if ( tPIR == 0 ) {
    digitalWrite(BUILTIN_LED, HIGH);
    delay(500);
    digitalWrite(BUILTIN_LED, LOW);
    delay(500);
  } else {
    delay(1000);
  }
  tPIR++;
  setLED(brightness);
  if (fanOn) {
    if (keepFanOn == 1) {
      // use ledInactivityLimit instead of fanInactivityLimit
      if ( tPIR > ledInactivityLimit) {
        digitalWrite(RELAY, LOW);
        fanOn = false;
      }
    } else { 
      if ( tPIR > fanInactivityLimit) {
        digitalWrite(RELAY, LOW);
        fanOn = false;
      }
    }
  }
  if (ledOn && tPIR > ledInactivityLimit ) {
    digitalWrite(NEOPIXEL, LOW);
    ledOn = false;
  }
}
