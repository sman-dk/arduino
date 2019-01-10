#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <WiFiManager.h>   

#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#define MQTT_SERVER      "mqtt.sman.dk"
#define MQTT_SERVERPORT  1883                   // 8883 for MQTTS
#define MQTT_USERNAME    "user1"
#define MQTT_KEY         "bjarne"

#define BTN_PREV     D1
#define BTN_PLAY     D2
#define BTN_PAUSE    D3
#define BTN_NEXT     D6
#define BTN_VOL_UP   D7
#define BTN_VOL_DOWN D8 // 10k pull down

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_SERVERPORT, MQTT_USERNAME, MQTT_KEY);

// Setup a feed called 'photocell' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish spotify = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME "/georg/spotify");

// Setup a feed called 'onoff' for subscribing to changes.
//Adafruit_MQTT_Subscribe display = Adafruit_MQTT_Subscribe(&mqtt, MQTT_USERNAME "/georg/display");

void setup() {
  // put your setup code here, to run once:
  //Setup Serial port speed
  Serial.begin(115200);
  Serial.println("Starting WiFiManager");
  // put your setup code here, to run once:
  WiFiManager wifiManager;
  // Uncomment and run it once, if you want to erase all the stored information
  //wifiManager.resetSettings();
  wifiManager.autoConnect("hest");

  // Setup MQTT subscription for display feed.
  //mqtt.subscribe(&display);
}

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();
  
  // put your main code here, to run repeatedly:
  if (! spotify.publish("play")) {
    Serial.println("Failed");
  } else {
    Serial.println("OK!");
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
