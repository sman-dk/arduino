#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)

#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <OneWire.h>              // MAX31850 onewire by Adafruit
#include <DallasTemperature.h>    // The non-adafruit version

//DS18B20
#define ONE_WIRE_BUS_A D3 //Pin to which is attached a temperature sensor
#define ONE_WIRE_BUS_B D4 //Pin to which is attached a temperature sensor
#define ONE_WIRE_MAX_DEV 15 //The maximum number of devices

OneWire oneWire_A(ONE_WIRE_BUS_A);
OneWire oneWire_B(ONE_WIRE_BUS_B);
DallasTemperature DS18B20_A(&oneWire_A);
DallasTemperature DS18B20_B(&oneWire_B);
int numberOfDevices_A; //Number of temperature devices found
int numberOfDevices_B; //Number of temperature devices found
DeviceAddress devAddr_A[ONE_WIRE_MAX_DEV];  //An array device temperature sensors
DeviceAddress devAddr_B[ONE_WIRE_MAX_DEV];  //An array device temperature sensors
float tempDev_A[ONE_WIRE_MAX_DEV]; //Saving the last measurement of temperature
float tempDev_B[ONE_WIRE_MAX_DEV]; //Saving the last measurement of temperature
float tempDevLast_A[ONE_WIRE_MAX_DEV]; //Previous temperature measurement
float tempDevLast_B[ONE_WIRE_MAX_DEV]; //Previous temperature measurement
long lastTemp; //The last measurement
const int durationTemp = 5000; //The frequency of temperature measurement


//HTTP
ESP8266WebServer server(80);

//------------------------------------------
//Convert device id to String
String GetAddressToString(DeviceAddress deviceAddress){
  String str = "";
  for (uint8_t i = 0; i < 8; i++){
    if( deviceAddress[i] < 16 ) str += String(0, HEX);
    str += String(deviceAddress[i], HEX);
  }
  return str;
}

//Setting the temperature sensor
void SetupDS18B20(){
  DS18B20_A.begin();
  DS18B20_B.begin();

  Serial.print("Parasite power for A is: "); 
  if( DS18B20_A.isParasitePowerMode() ){ 
    Serial.println("ON");
  }else{
    Serial.println("OFF");
  }

  Serial.print("Parasite power for B is: "); 
  if( DS18B20_B.isParasitePowerMode() ){ 
    Serial.println("ON");
  }else{
    Serial.println("OFF");
  }
  
  numberOfDevices_A = DS18B20_A.getDeviceCount();
  numberOfDevices_B = DS18B20_B.getDeviceCount();
  Serial.print( "Device count A: " );
  Serial.println( numberOfDevices_A );
  Serial.print( "Device count B: " );
  Serial.println( numberOfDevices_B );

  lastTemp = millis();
  DS18B20_A.requestTemperatures();
  DS18B20_B.requestTemperatures();

  // Loop through each device, print out address
  for(int i=0;i<numberOfDevices_A; i++){
    // Search the wire for address
    if( DS18B20_A.getAddress(devAddr_A[i], i) ){
      //devAddr[i] = tempDeviceAddress;
      Serial.print("Found device (A) ");
      Serial.print(i, DEC);
      Serial.print(" with address: " + GetAddressToString(devAddr_A[i]));
      Serial.println();
    }else{
      Serial.print("Found ghost device at (A) ");
      Serial.print(i, DEC);
      Serial.print(" but could not detect address. Check power and cabling");
    }

    //Get resolution of DS18b20
    Serial.print("Resolution: ");
    Serial.print(DS18B20_A.getResolution( devAddr_A[i] ));
    Serial.println();

    //Read temperature from DS18b20
    float tempC = DS18B20_A.getTempC( devAddr_A[i] );
    Serial.print("Temp C: ");
    Serial.println(tempC);
  }

   // Loop through each device, print out address
  for(int i=0;i<numberOfDevices_B; i++){
    // Search the wire for address
    if( DS18B20_B.getAddress(devAddr_B[i], i) ){
      //devAddr[i] = tempDeviceAddress;
      Serial.print("Found device (A) ");
      Serial.print(i, DEC);
      Serial.print(" with address: " + GetAddressToString(devAddr_B[i]));
      Serial.println();
    }else{
      Serial.print("Found ghost device at (A) ");
      Serial.print(i, DEC);
      Serial.print(" but could not detect address. Check power and cabling");
    }

    //Get resolution of DS18b20
    Serial.print("Resolution: ");
    Serial.print(DS18B20_B.getResolution( devAddr_B[i] ));
    Serial.println();

    //Read temperature from DS18b20
    float tempC = DS18B20_B.getTempC( devAddr_B[i] );
    Serial.print("Temp C: ");
    Serial.println(tempC);
  }
}

//Loop measuring the temperature
void TempLoop(long now){
  if( now - lastTemp > durationTemp ){ //Take a measurement at a fixed time (durationTemp = 5000ms, 5s)
    // A:
    for(int i=0; i<numberOfDevices_A; i++){
      float tempC = DS18B20_A.getTempC( devAddr_A[i] ); //Measuring temperature in Celsius
      tempDev_A[i] = tempC; //Save the measured value to the array
    }
    DS18B20_A.setWaitForConversion(false); //No waiting for measurement
    DS18B20_A.requestTemperatures(); //Initiate the temperature measurement
    // B:
    for(int i=0; i<numberOfDevices_B; i++){
      float tempC = DS18B20_B.getTempC( devAddr_B[i] ); //Measuring temperature in Celsius
      tempDev_B[i] = tempC; //Save the measured value to the array
    }
    DS18B20_B.setWaitForConversion(false); //No waiting for measurement
    DS18B20_B.requestTemperatures(); //Initiate the temperature measurement
    
    lastTemp = millis();  //Remember the last time measurement
  }
}

//------------------------------------------
void HandleRoot(){
  char temperatureString[6];
  // A:
  String message = "<h3>Dallas 1-wire (ds18b20) temperature readings</h3>\r\n";
  message += "Number of devices (bus A): ";
  message += numberOfDevices_A;
  message += "\r\n<br>";
  for(int i=0;i<numberOfDevices_A;i++){
    dtostrf(tempDev_A[i], 2, 2, temperatureString);
    Serial.print( "Sending temperature: " );
    Serial.println( temperatureString );
    message += "<i>";
    message += GetAddressToString( devAddr_A[i] );
    message += ": </i>";
    message += "<b>";
    message += temperatureString;
    message += " [C]</b><br>\r\n";
  }

  message += "Number of devices (bus B): ";
  message += numberOfDevices_B;
  message += "\r\n<br>";
  for(int i=0;i<numberOfDevices_B;i++){
    dtostrf(tempDev_B[i], 2, 2, temperatureString);
    Serial.print( "Sending temperature: " );
    Serial.println( temperatureString );

    message += "<i>";
    message += GetAddressToString( devAddr_B[i] );
    message += ": </i>";
    message += "<b>";
    message += temperatureString;
    message += " [C]</b><br>\r\n";
  }
  message += "\r\n";
  
  server.send(200, "text/html", message );
}

void HandleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/html", message);
}


void setup() {
  // put your setup code here, to run once:
  WiFiManager wifiManager;
  wifiManager.autoConnect();

  //Setup Serial port speed
  Serial.begin(115200);
  Serial.println("Hello world");

  server.on("/", HandleRoot);
  server.onNotFound( HandleNotFound );
  server.begin();
  Serial.println("HTTP server started");
  Serial.println(WiFi.softAPIP());

  //Setup DS18b20 temperature sensor
  SetupDS18B20();
}

void loop() {
  // ds18b20 stuff originally from https://www.hackster.io/adachsoft_com/esp8266-temperature-sensors-ds18b20-with-http-server-5509ac
  long t = millis();
  
  server.handleClient();
  TempLoop( t );
}
