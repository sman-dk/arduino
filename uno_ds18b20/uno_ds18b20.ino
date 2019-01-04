// Includes borrowed code from the internet and Arduino built in examples and is generally a bit messy (A,B,C..).. but it works fine :-)

#include <OneWire.h>              // MAX31850 onewire by Adafruit
#include <DallasTemperature.h>    // The non-adafruit version
// Ethernet
#include <SPI.h>
#include <Ethernet.h>

byte mac[] = {
  0xFE, 0xAD, 0xBE, 0xEF, 0xFA, 0xAD
};
IPAddress ip(10, 102, 0, 31);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

//DS18B20
#define ONE_WIRE_BUS_A 3 //Pin to which is attached a temperature sensor
#define ONE_WIRE_BUS_B 4 //Pin to which is attached a temperature sensor
#define ONE_WIRE_BUS_C 5 //Pin to which is attached a temperature sensor
#define ONE_WIRE_MAX_DEV 10 //The maximum number of devices

OneWire oneWire_A(ONE_WIRE_BUS_A);
OneWire oneWire_B(ONE_WIRE_BUS_B);
OneWire oneWire_C(ONE_WIRE_BUS_C);
DallasTemperature DS18B20_A(&oneWire_A);
DallasTemperature DS18B20_B(&oneWire_B);
DallasTemperature DS18B20_C(&oneWire_C);
int numberOfDevices_A; //Number of temperature devices found
int numberOfDevices_B; //Number of temperature devices found
int numberOfDevices_C; //Number of temperature devices found
DeviceAddress devAddr_A[ONE_WIRE_MAX_DEV];  //An array device temperature sensors
DeviceAddress devAddr_B[ONE_WIRE_MAX_DEV];  //An array device temperature sensors
DeviceAddress devAddr_C[ONE_WIRE_MAX_DEV];  
float tempDev_A[ONE_WIRE_MAX_DEV]; //Saving the last measurement of temperature
float tempDev_B[ONE_WIRE_MAX_DEV];
float tempDev_C[ONE_WIRE_MAX_DEV];//Saving the last measurement of temperature
float tempDevLast_A[ONE_WIRE_MAX_DEV]; //Previous temperature measurement
float tempDevLast_B[ONE_WIRE_MAX_DEV];
float tempDevLast_C[ONE_WIRE_MAX_DEV];//Previous temperature measurement
long lastTemp; //The last measurement
const int durationTemp = 5000; //The frequency of temperature measurement


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
  DS18B20_C.begin();

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

  Serial.print("Parasite power for C is: "); 
  if( DS18B20_C.isParasitePowerMode() ){ 
    Serial.println("ON");
  }else{
    Serial.println("OFF");
  }
  
  numberOfDevices_A = DS18B20_A.getDeviceCount();
  numberOfDevices_B = DS18B20_B.getDeviceCount();
  numberOfDevices_C = DS18B20_C.getDeviceCount();
  Serial.print( "Device count A: " );
  Serial.println( numberOfDevices_A );
  Serial.print( "Device count B: " );
  Serial.println( numberOfDevices_B );
  Serial.print( "Device count C: " );
  Serial.println( numberOfDevices_C );
  lastTemp = millis();

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
      Serial.print("Found device (B) ");
      Serial.print(i, DEC);
      Serial.print(" with address: " + GetAddressToString(devAddr_B[i]));
      Serial.println();
    }else{
      Serial.print("Found ghost device at (B) ");
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

    // Loop through each device, print out address
  for(int i=0;i<numberOfDevices_C; i++){
    // Search the wire for address
    if( DS18B20_C.getAddress(devAddr_C[i], i) ){
      //devAddr[i] = tempDeviceAddress;
      Serial.print("Found device (C) ");
      Serial.print(i, DEC);
      Serial.print(" with address: " + GetAddressToString(devAddr_C[i]));
      Serial.println();
    }else{
      Serial.print("Found ghost device at (C) ");
      Serial.print(i, DEC);
      Serial.print(" but could not detect address. Check power and cabling");
    }

    //Get resolution of DS18b20
    Serial.print("Resolution: ");
    Serial.print(DS18B20_C.getResolution( devAddr_C[i] ));
    Serial.println();

    //Read temperature from DS18b20
    float tempC = DS18B20_C.getTempC( devAddr_C[i] );
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
    // C:
    for(int i=0; i<numberOfDevices_C; i++){
      float tempC = DS18B20_C.getTempC( devAddr_B[i] ); //Measuring temperature in Celsius
      tempDev_C[i] = tempC; //Save the measured value to the array
    }
    DS18B20_C.setWaitForConversion(false); //No waiting for measurement
    DS18B20_C.requestTemperatures(); //Initiate the temperature measurement
    
    lastTemp = millis();  //Remember the last time measurement
  }
}

//------------------------------------------


void setup() {
  // put your setup code here, to run once:
  Ethernet.init(10);  // Most Arduino shields
  //Setup Serial port speed
  Serial.begin(115200);
  Serial.println("Hello world");

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  // start the server
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  //Setup DS18b20 temperature sensors
  SetupDS18B20();
}

void loop() {
  // ds18b20 stuff originally from https://www.hackster.io/adachsoft_com/esp8266-temperature-sensors-ds18b20-with-http-server-5509ac
  long t = millis();
  TempLoop( t );
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 10");  // refresh the page automatically every 5 sec
          client.println();
          

          char temperatureString[6];
          // A:
          String message = "<h3>Dallas 1-wire (ds18b20) temperature readings</h3>\r\n";
          client.println("Number of devices (bus A): ");
          client.println(numberOfDevices_A);
          client.println("<br>");
          for(int i=0;i<numberOfDevices_A;i++){
            dtostrf(tempDev_A[i], 2, 2, temperatureString);
            Serial.print( "Sending temperature: " );
            Serial.println( temperatureString );
            client.print("<i>");
            client.print(GetAddressToString( devAddr_A[i] ));
            client.print(": </i>");
            client.print("<b>");
            client.print(temperatureString);
            client.print(" [C]</b><br>\r\n");
          }
          // B
          client.print("Number of devices (bus B): ");
          client.print(numberOfDevices_B);
          client.print("\r\n<br>");
          for(int i=0;i<numberOfDevices_B;i++){
            dtostrf(tempDev_B[i], 2, 2, temperatureString);
            Serial.print( "Sending temperature: " );
            Serial.println( temperatureString );
        
            client.print("<i>");
            client.print(GetAddressToString( devAddr_B[i] ));
            client.print(": </i>");
            client.print("<b>");
            client.print(temperatureString);
            client.print(" [C]</b><br>\r\n");
          }
          // C
          client.print("Number of devices (bus C): ");
          client.print(numberOfDevices_C);
          client.print("\r\n<br>");
          for(int i=0;i<numberOfDevices_C;i++){
            dtostrf(tempDev_C[i], 2, 2, temperatureString);
            Serial.print( "Sending temperature: " );
            Serial.println( temperatureString );
        
            client.print("<i>");
            client.print(GetAddressToString( devAddr_C[i] ));
            client.print(": </i>");
            client.print("<b>");
            client.print(temperatureString);
            client.print(" [C]</b><br>\r\n");
          }
          client.print("\r\n");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}
