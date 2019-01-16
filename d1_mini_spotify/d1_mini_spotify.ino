#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <WiFiManager.h>   

#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#define MQTT_SERVER      "mqtt.sman.dk"
#define MQTT_SERVERPORT  1883
#define MQTT_USERNAME    "user"
#define MQTT_KEY         "pass"
// d4 is wired to led the LED. Don't use that.
// D0 does not have interrupt capability
#define BTN_PREV     D8
#define BTN_START    D7
#define BTN_PAUSE    D6
#define BTN_NEXT     D5
#define BTN_VOL_UP   D1
#define BTN_VOL_DWN  D2

const unsigned int sleepTime = 10; // For the main loop
const unsigned int debounceDelay = 50; // simple debounce for buttons
const unsigned int maxBtnTime = 700;
unsigned long long deltaTime = 0;
boolean prevInterrupt = false;
boolean startInterrupt = false;
boolean pauseInterrupt = false;
boolean nextInterrupt = false;
boolean volUpInterrupt = false;
boolean volDwnInterrupt = false;
boolean prevMsgSent = false;
boolean startMsgSent = false;
boolean pauseMsgSent = false;
boolean nextMsgSent = false;
boolean volUpMsgSent = false;
boolean volDwnMsgSent = false;
unsigned long long prevPressedTime = 0;
unsigned long long startPressedTime = 0;
unsigned long long pausePressedTime = 0;
unsigned long long nextPressedTime = 0;
unsigned long long volUpPressedTime = 0;
unsigned long long volDwnPressedTime = 0;

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_SERVERPORT, MQTT_USERNAME, MQTT_KEY);

// Setup a feed called 'photocell' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish spotify = Adafruit_MQTT_Publish(&mqtt, "georg/spotify");

// Setup a feed called 'onoff' for subscribing to changes.
//Adafruit_MQTT_Subscribe display = Adafruit_MQTT_Subscribe(&mqtt, "georg/display");

void setup() {
  // put your setup code here, to run once:
  //Setup Serial port speed
  Serial.begin(115200);
  Serial.println("Starting WiFiManager");
  // put your setup code here, to run once:
  WiFiManager wifiManager;
  // Uncomment and run it once, if you want to erase all the stored information
  //wifiManager.resetSettings();
  wifiManager.autoConnect("spotify");

  // Setup MQTT subscription for display feed.
  //mqtt.subscribe(&display);
   pinMode(BTN_PREV, INPUT);
   pinMode(BTN_START, INPUT);
   pinMode(BTN_PAUSE, INPUT);
   pinMode(BTN_NEXT, INPUT);
   pinMode(BTN_VOL_UP, INPUT);
   pinMode(BTN_VOL_DWN, INPUT);
   attachInterrupt(BTN_PREV, prev, FALLING);
   attachInterrupt(BTN_START, start, FALLING);
   attachInterrupt(BTN_PAUSE, pause, FALLING);
   attachInterrupt(BTN_NEXT, next, FALLING);
   attachInterrupt(BTN_VOL_UP, vol_up, FALLING);
   attachInterrupt(BTN_VOL_DWN, vol_dwn, FALLING);
}

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();

void prev (){
  if ( prevInterrupt == false ) {
    prevInterrupt = true;
    prevMsgSent = false;
    prevPressedTime = millis();
  }
}

void start (){
  if ( startInterrupt == false ) {
    startInterrupt = true;
    startMsgSent = false;
    startPressedTime = millis();
  }
}

void pause (){
  if ( pauseInterrupt == false ) {
    pauseInterrupt = true;
    pauseMsgSent = false;
    pausePressedTime = millis();
  }
}

void next (){
  if ( nextInterrupt == false ) {
    nextInterrupt = true;
    nextMsgSent = false;
    nextPressedTime = millis();
  }
}

void vol_dwn (){
  if ( volDwnInterrupt == false ) {
    volDwnInterrupt = true;
    volDwnMsgSent = false;
    volDwnPressedTime = millis();
  }
}

void vol_up (){
  if ( volUpInterrupt == false ) {
    volUpInterrupt = true;
    volUpMsgSent = false;
    volUpPressedTime = millis();
  }
}

void loop() {
  delay(sleepTime);
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();
  
  // BTN_PREV
  if (prevInterrupt == true ) {
    if (digitalRead(BTN_PREV) == HIGH ) {
      prevInterrupt = false;
    } else {
      deltaTime = millis() - prevPressedTime;
      if ( deltaTime > debounceDelay and deltaTime < maxBtnTime  and not prevMsgSent) {
        if ( spotify.publish("prev")) {
          prevMsgSent = true;
          Serial.println("Sending mqtt msg \"prev\" ok!");
        } else {
          Serial.println("Sending mqtt msg \"prev\" failed!");
        }
      } else if ( deltaTime >= maxBtnTime ) {
        prevInterrupt = false;
      }
    }
  }

  // BTN_START
  if (startInterrupt == true ) {
    if (digitalRead(BTN_START) == HIGH ) {
      startInterrupt = false;
    } else {
      deltaTime = millis() - startPressedTime;
      if ( deltaTime > debounceDelay and deltaTime < maxBtnTime  and not startMsgSent) {
        if ( spotify.publish("start")) {
          startMsgSent = true;
          Serial.println("Sending mqtt msg \"start\" ok!");
        } else {
          Serial.println("Sending mqtt msg \"start\" failed!");
        }
      } else if ( deltaTime >= maxBtnTime ) {
        startInterrupt = false;
      }
    }
  }

  // BTN_PAUSE
  if (pauseInterrupt == true ) {
    if (digitalRead(BTN_PAUSE) == HIGH ) {
      pauseInterrupt = false;
    } else {
      deltaTime = millis() - pausePressedTime;
      if ( deltaTime > debounceDelay and deltaTime < maxBtnTime  and not pauseMsgSent) {
        if ( spotify.publish("pause")) {
          pauseMsgSent = true;
          Serial.println("Sending mqtt msg \"pause\" ok!");
        } else {
          Serial.println("Sending mqtt msg \"pause\" failed!");
        }
      } else if ( deltaTime >= maxBtnTime ) {
        pauseInterrupt = false;
      }
    }
  }

  // BTN_NEXT
  if (nextInterrupt == true ) {
    if (digitalRead(BTN_NEXT) == HIGH ) {
      nextInterrupt = false;
    } else {
      deltaTime = millis() - nextPressedTime;
      if ( deltaTime > debounceDelay and deltaTime < maxBtnTime  and not nextMsgSent) {
        if ( spotify.publish("next")) {
          nextMsgSent = true;
          Serial.println("Sending mqtt msg \"next\" ok!");
        } else {
          Serial.println("Sending mqtt msg \"next\" failed!");
        }
      } else if ( deltaTime >= maxBtnTime ) {
        nextInterrupt = false;
      }
    }
  }

  // BTN_VOL_UP
  if (volUpInterrupt == true ) {
    if (digitalRead(BTN_VOL_UP) == HIGH ) {
      volUpInterrupt = false;
    } else {
      deltaTime = millis() - volUpPressedTime;
      if ( deltaTime > debounceDelay and deltaTime < maxBtnTime and not volUpMsgSent ) {
        if ( spotify.publish("vol_up")) {
          volUpMsgSent = true;
          Serial.println("Sending mqtt msg \"vol_up\" ok!");
        } else {
          Serial.println("Sending mqtt msg \"vol_up\" failed!");
        }
      } else if ( deltaTime >= maxBtnTime ) {
        volUpInterrupt = false;
      }
    }
  }

  // BTN_VOL_DWN
  if (volDwnInterrupt == true ) {
    if (digitalRead(BTN_VOL_DWN) == HIGH ) {
      volDwnInterrupt = false;
    } else {
      deltaTime = millis() - volDwnPressedTime;
      if ( deltaTime > debounceDelay and deltaTime < maxBtnTime  and not volDwnMsgSent) {
        if ( spotify.publish("vol_dwn")) {
          volDwnMsgSent = true;
          Serial.println("Sending mqtt msg \"vol_dwn\" ok!");
        } else {
          Serial.println("Sending mqtt msg \"vol_dwn\" failed!");
        }
      } else if ( deltaTime >= maxBtnTime ) {
        volDwnInterrupt = false;
      }
    }
  }
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}
