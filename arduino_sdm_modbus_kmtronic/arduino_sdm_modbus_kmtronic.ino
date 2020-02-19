/* 
 *  Note2self: Parts of this code is a bit messy. Loats of globals and even a goto.. but hey it works.
 *  
 *  RS485 stuff inspired from KMTRONIC LTD/kmtronic.com: https://web-relays.com/en/blog/arduino-code-read-voltage-from-eastron-sdm230-modbus-smart-energy-meter-with-modbus-protocol/
 * 
 * SDM reference: https://bg-etech.de/download/manual/SDM630Register1-5.pdf
 * Set address using e.g. https://github.com/nickma82/eastron_setuptool
 * 
 * Watchdog stuff: 
 * https://tushev.org/articles/arduino/5/arduino-and-watchdog-timer
 * 
 *
*/
// Ethershield webserver

// please modify the following two lines. mac and ip have to be unique
// in your local area network. You can not have the same numbers in
// two devices:
static uint8_t mymac[6] = {
  0x54,0x55,0x58,0x11,0x00,0x44}; 
  
static uint8_t myip[4] = {
  10,99,0,99};
  
// Digital pins
#define RELAY1 8
#define RELAY2 7
#define RELAY3 6
#define RELAY4 5

// Analog inputs
#define INPUT1 A5
#define INPUT2 A4
#define INPUT3 A3
#define INPUT4 A2

// RS485 Control pin
#define RS485CONTROL 3

// Receive buffer
char Buffer[9];

union
{ 
 unsigned long a ;
 float b ;
} u ;

// The function crc_fn is grabbed from https://arduino.stackexchange.com/questions/25031/modbus-protocol-rtu
int CRC16;
char CRC;
unsigned int crc_fn(unsigned char *dpacket,unsigned int len)    // CRC 16 Function(Error calculation)
    {
        unsigned int crc = 0xffff,poly = 0xa001;
        unsigned char i=0,j=0;

        for(i=0;i<len;i++)
        {
            crc^= dpacket[i];
            for(j=0;j<8;j++)
            {
                if(crc & 0x01)
                {
                    crc >>= 1;
                    crc ^= poly;
                }
                else
                crc >>= 1;
            }
        }
        return (crc);
    }

char crc_lo(unsigned char *dpacket, char len)
    {
      CRC16 = crc_fn(dpacket, len);
      CRC = CRC16;
      return CRC;
    }

char crc_hi(unsigned char *dpacket, char len)
    {
      CRC16 = crc_fn(dpacket, len);
      CRC = CRC16>>8;
      return CRC;
    }

// This command is to read 2 registers (4 bytes) from meter 
// (Most of the bytes will be changed in the code anyway)
char readReq [] = {0x01, 0x04, 0x01, 0x56, 0x00, 0x02, 0xf1, 0xee};
char exceptionMsg[72];
float meterVal;

// From https://bg-etech.de/download/manual/SDM630Register1-5.pdf

const char * const mReg [][5] PROGMEM = {
  {"L1V",         0x00, 0x00, "L1 Voltage", "V"},
  {"L2V",         0x00, 0x02, "L2 Voltage", "V"},
  {"L3V",         0x00, 0x04, "L3 Voltage", "V"},
  {"L1A",         0x00, 0x06, "L1 Current", "A"},
  {"L2A",         0x00, 0x08, "L2 Current", "A"},
  {"L3A",         0x00, 0x0A, "L3 Current", "A"},
  {"L1W",         0x00, 0x0C, "L1 Power", "W"},
  {"L2W",         0x00, 0x0E, "L2 Power", "W"},
  {"L3W",         0x00, 0x10, "L3 Power", "W"},
  {"L1VA",        0x00, 0x12, "L1 Apparent power", "VA"},
  {"L2VA",        0x00, 0x14, "L2 Apparent power", "VA"},
  {"L3VA",        0x00, 0x16, "L3 Apparent power", "VA"},
  {"L1VAr",       0x00, 0x18, "L1 Reactive power", "VAr"},
  {"L2VAr",       0x00, 0x1A, "L2 Reactive power", "VAr"},
  {"L3VAr",       0x00, 0x1C, "L3 Reactive power", "VAr"},
  {"L1pf",        0x00, 0x1E, "L1 Power factor", ""},
  {"L2pf",        0x00, 0x20, "L2 Power factor", ""},
  {"L3pf",        0x00, 0x22, "L3 Power factor", ""},
  {"L1ang",       0x00, 0x24, "L1 Phase angle", "°"},
  {"L2ang",       0x00, 0x26, "L2 Phase angle", "°"},
  {"L3ang",       0x00, 0x28, "L3 Phase angle", "°"},
  {"AvgV",        0x00, 0x2A, "Avg. voltage (all phases?)", "V"},
  {"AvgA",        0x00, 0x2E, "Avg. current (all phases?)", "A"},
  {"SumA",        0x00, 0x30, "Sum of currents", "A"},
  {"TotW",        0x00, 0x34, "Total power", "W"},
  {"TotVA",       0x00, 0x38, "Total power", "VA"},
  {"Totvar",      0x00, 0x3C, "Total power", "var"},
  {"Totpf",       0x00, 0x3E, "Total power factor", ""},
  {"Totang",      0x00, 0x42, "Total phase angle", "°"},
  {"TotHz",       0x00, 0x46, "Frequency", "Hz"},
  {"TotImpkWh",   0x00, 0x48, "Total import kWh", "kWh"},
  {"TotExpkWh",   0x00, 0x4A, "Total export kWh", "kWh"},
  {"TotImpkVArh", 0x00, 0x4C, "Total import kVArh", "kVArh"},
  {"TotExpkVArh", 0x00, 0x4E, "Total export kVArh", "kVArh"},
  {"TotkVAh",     0x00, 0x50, "Total kVAh", "kVAh"},
  {"Ah",          0x00, 0x52, "Ah", "Ah"},
  {"TotWdemand",  0x00, 0x54, "Total power demand (import - export)", "W"},
  {"MaxTotWdemand",  0x00, 0x56, "Max. total power demand (import - export)", "W"},
  {"TotVAdemand",    0x00, 0x64, "Total VA demand (import - export)", "VA"},
  {"MaxTotVAdemand", 0x00, 0x66, "Max. total VA demand (import - export)", "VA"},
  {"TotNAdemand",    0x00, 0x68, "Total neutral current demand (import - export)", "A"},
  {"MaxTotNAdemand", 0x00, 0x6A, "Max. neutral current demand (import - export)", "A"},
  {"L1L2V", 0x00, 0xC8, "L1 to L2 voltage", "V"},
  {"L2L3V", 0x00, 0xCA, "L2 to L3 voltate", "V"},
  {"L3L1V", 0x00, 0xCC, "L3 to L1 voltage", "V"},
  {"AvgLLV", 0x00, 0xCE, "Average line to line voltage", "V"},
  {"NA",     0x00, 0xE0, "Neutral current", "A"},
  {"L1VTHD", 0x00, 0xEA, "L1/N voltage THD", "%"},
  {"L2VTHD", 0x00, 0xEC, "L2/N voltage THD", "%"},
  {"L3VTHD", 0x00, 0xEE, "L3/N voltage THD", "%"},
  {"L1ATHD", 0x00, 0xF0, "L1 current THD", "%"},
  {"L2ATHD", 0x00, 0xF2, "L2 current THD", "%"},
  {"L3ATHD", 0x00, 0xF4, "L3 current THD", "%"},
  {"AvgLVTHD", 0x00, 0xF8, "Avg. L to N voltage THD", "%"},
  {"AvgLATHD", 0x00, 0xF8, "Avg. line current THD", "%"},
  {"L1Ademand", 0x01, 0x02, "L1 current demand (import - export)", "A"},
  {"L2Ademand", 0x01, 0x04, "L2 current demand (import - export)", "A"},
  {"L3Ademand", 0x01, 0x06, "L3 current demand (import - export)", "A"},
  {"MaxL1Ademand", 0x01, 0x08, "Max. L1 current demand (import - export)", "A"},
  {"MaxL2Ademand", 0x01, 0x0A, "Max. L2 current demand (import - export)", "A"},
  {"MaxL3Ademand", 0x01, 0x0C, "Max. L3 current demand (import - export)", "A"},
  {"L1L2VTHD", 0x01, 0x4E, "L1/L2 voltage THD", "%"},
  {"L2L3VTHD", 0x01, 0x50, "L2/L3 voltage THD", "%"},
  {"L3L1VTHD", 0x01, 0x52, "L3/L1 voltage THD", "%"},
  {"AvgLLVTHD", 0x01, 0x54, "Average line to neutral voltage THD", "%"},  
  {"TotkWh",    0x01, 0x56, "Total kWh (import - export)", "kWh"},
  {"TotkVArh",  0x01, 0x58, "Total kVArh (import - export)", "kVArh"},
  {"L1impkWh", 0x01, 0x5A, "L1 import kWh", "kWh"},
  {"L2impkWh", 0x01, 0x5C, "L2 import kWh", "kWh"},
  {"L3impkWh", 0x01, 0x5E, "L3 import kWh", "kWh"},
  {"L1expkWh", 0x01, 0x60, "L1 export kWh", "kWh"},
  {"L2expkWh", 0x01, 0x62, "L2 export kWh", "kWh"},
  {"L3expkWh", 0x01, 0x64, "L3 export kWh", "kWh"},
  {"L1totkWh", 0x01, 0x66, "L1 total kWh (import - export)", "kWh"},
  {"L2totkWh", 0x01, 0x68, "L2 total kWh (import - export)", "kWh"},
  {"L3totkWh", 0x01, 0x6A, "L3 total kWh (import - export)", "kWh"},
  {"L1impkVArh", 0x01, 0x6C, "L1 import kVArh", "kVArh"},
  {"L2impkVArh", 0x01, 0x6E, "L2 import kVArh", "kVArh"},
  {"L3impkVArh", 0x01, 0x70, "L3 import kVArh", "kVArh"},
  {"L1expkVArh", 0x01, 0x72, "L1 export kVArh", "kVArh"},
  {"L2expkVArh", 0x01, 0x74, "L2 export kVArh", "kVArh"},
  {"L3expkVArh", 0x01, 0x76, "L3 export kVArh", "kVArh"},
  {"L1totkVArh", 0x01, 0x78, "L1 total kVArh (import - export)", "kVArh"},
  {"L2totkVArh", 0x01, 0x7A, "L2 total kVArh (import - export)", "kVArh"},
  {"L3totkVArh", 0x01, 0x7C, "L3 total kVArh (import - export)", "kVArh"},
  {"STOP", 0x00, 0x00, "end of array :-)", ""},
};

uint8_t mAddr; // Modbus address 
String sAddr; // Modbus address
String sReqType;
int modbusResult;
boolean pathOk = false;
uint16_t plen, dat_p; // Web server stuff
char reqType[16];

// Sauna on button. If set the on button (input2) may be used
boolean saunaOnButtonActive = false;
// simple debounce for buttons
const unsigned int debounceDelay = 50;
const unsigned int maxBtnTime = 3000;
unsigned long long deltaTime = 0;
unsigned long long onBtnPressedTime = 0;
unsigned long long offBtnPressedTime = 0;
int inputThreshold = 600;

// EtherShield webserver
#define prog_char char PROGMEM
#include "EtherShield.h"

// Watchdog
// https://tushev.org/articles/arduino/5/arduino-and-watchdog-timer
#include <avr/wdt.h>

#define MYWWWPORT 80
#define BUFFER_SIZE 550
static uint8_t buf[BUFFER_SIZE+1];

// The ethernet shield
EtherShield es=EtherShield();

uint16_t http200ok(void)
{
  return(es.ES_fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n")));
}

uint16_t http404(void)
{
  return(es.ES_fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 404 Not Found\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n")));
}

// prepare the webpage by writing the data to the tcp send buffer
uint16_t print_webpage(uint8_t *buf)
{
  uint16_t plen;
  plen=http200ok();
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<html><head><title>Vammen Camping electricity reader stuff</title></head><body>"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<h1>Welcome</h1>"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("Uptime: "));
  char uptime[7] = {0, 0, 0, 0, 0, 0, 0};
  unsigned long mUp = millis()/1000;
  ltoa(mUp, uptime, 7);
  plen=es.ES_fill_tcp_data_len(buf,plen, uptime, 7);
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR(" s"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("</body></html>\n"));

  return(plen);
}

uint16_t invalid_path_response(uint8_t *buf)
{
  uint16_t plen;
  plen=http404();
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("Invalid path\n"));
  return(plen);
}

// Sending a packet
int sendModbusPacket() {
  *exceptionMsg = "";
  memset(Buffer, 0, sizeof(Buffer));
  meterVal = NULL;
 // Calculate CRC
 readReq[6] = crc_lo(readReq, 6);
 readReq[7] = crc_hi(readReq, 6);
 // Set pin to HIGH > Transmit mode
 digitalWrite(RS485CONTROL, HIGH); 
 // Empty receive buffer (if any.. it should be empty)
 while(Serial.available()) Serial.read();
 // Send command to ModBus Meter
 for (int i=0; i < 8; i++) Serial.write(readReq[ i ]);
 delay(50);
 // Set pin to LOW > Receive mode
 digitalWrite(RS485CONTROL, LOW);
 // Waiting for receive reply with built in timeout
 for ( int t = 0; t < 50 ; t++) {
  // We received what we wanted
  if ( Serial.available() == 9) break;
  delay(10);
 }
 // A normal response
 if ( Serial.available() == 9) {
  // A normal response
  for (char i=0; i < 9; i++) Buffer[i] = Serial.read();
  // CRC check
  if ( crc_lo(Buffer, 7) == Buffer[7] && crc_hi(Buffer, 7) == Buffer[8]) {
   // CRC ok
   // Convert to Float
   ((byte*)&meterVal)[3]= Buffer[3];
   ((byte*)&meterVal)[2]= Buffer[4];
   ((byte*)&meterVal)[1]= Buffer[5];
   ((byte*)&meterVal)[0]= Buffer[6];
   return 0;
  } else {
    // CRC failed
    *exceptionMsg = "CRC failed for normal response";
   return 99;
  }
 } else if ( Serial.available() == 5) {
  // An exception response
  for (char i=0; i < 5; i++) Buffer[i] = Serial.read();
  if ( crc_lo(Buffer, 3) == Buffer[3] && crc_hi(Buffer, 3) == Buffer[4]) {
    // CRC ok
    // https://bg-etech.de/download/manual/SDM630Register1-5.pdf page 12
    if ( readReq[0] == Buffer[0] && ( readReq[1] | 0x80 ) == Buffer[1] ) {
      // slave address and function code is OK
      if ( Buffer[2] == 1 ) {
        *exceptionMsg = "Exception response: Illegal Function"; 
      } else if ( Buffer[2] == 2 ) {
        *exceptionMsg = "Exception response: Illegal Data Address"; 
      } else if ( Buffer[2] == 3 ) {
        *exceptionMsg = "Exception response: Illegal Data Value"; 
      } else if ( Buffer[2] == 5 ) {
        *exceptionMsg = "Exception response: Slave Device Failure"; 
      } else {
        *exceptionMsg = "Exception response: Undocumented error...";
      }
      return Buffer[2];
    } else {
      *exceptionMsg = "Exception packet error: CRC ok but wrong slave addr and/or function code";
      return 97;
    }
  } else {
    // CRC check for exception response failed
    *exceptionMsg = "Exception packet error: CRC failed";
    return 99;
  }
 } else if ( Serial.available() == 0) {
  *exceptionMsg = "Timout. No response on modbus.";
  return 98;
 } else {
  *exceptionMsg = "We got a response not 5 or 9 chars long. That should not happen";
  return 100 + Serial.available();
 }
}

void setup()
{
 // Watchdog
 wdt_enable(WDTO_8S);
 // Init serial communication
 Serial.begin(2400); 
 
 // Set RS485 Control pin to OUTPUT
 pinMode(RS485CONTROL, OUTPUT); 

 // Digital inputs setup
 pinMode(INPUT1, INPUT_PULLUP);
 pinMode(INPUT2, INPUT_PULLUP);
 pinMode(INPUT3, INPUT_PULLUP);
 pinMode(INPUT4, INPUT_PULLUP);
 
 // Set pin to send data HIGH
 digitalWrite(RS485CONTROL, HIGH);

 // Disable all outlets (then they are not active after a reboot/power outtage etc.)
 digitalWrite(RELAY1,0);
 digitalWrite(RELAY2,0);
 digitalWrite(RELAY3,0);
 digitalWrite(RELAY4,0);

  // initialize enc28j60
  es.ES_enc28j60Init(mymac);

  // init the ethernet/ip layer:
  es.ES_init_ip_arp_udp_tcp(mymac,myip, MYWWWPORT);

  Serial.println("VammenEL started..");
}

void loop()
{
  wdt_reset(); // Watchdog
    // A "while true", in order to be able to do a "continue" a few lines below..
    while (1) {
    wdt_reset(); // Watchdog
    // read packet, handle ping and wait for a tcp packet:
    dat_p=es.ES_packetloop_icmp_tcp(buf,es.ES_enc28j60PacketReceive(BUFFER_SIZE, buf));
    // Turn Sauna on
    if (analogRead(INPUT1) > inputThreshold && analogRead(INPUT2) < inputThreshold) {
      // Check if it may be used
      if (saunaOnButtonActive) {
        deltaTime = millis() - onBtnPressedTime;
        if ( deltaTime > debounceDelay and deltaTime < maxBtnTime) {
          digitalWrite(RELAY2, HIGH);
          onBtnPressedTime=0;
        } else if (deltaTime > maxBtnTime) {
          onBtnPressedTime = millis();
        }
      }
    }
    // Turn Suana off
    if (analogRead(INPUT2) > inputThreshold && analogRead(INPUT1) < inputThreshold) {
        deltaTime = millis() - offBtnPressedTime;
        if ( deltaTime > debounceDelay and deltaTime < maxBtnTime) {
          digitalWrite(RELAY2, LOW);
          offBtnPressedTime=0;
        } else if (deltaTime > maxBtnTime) {
          offBtnPressedTime = millis();
        }
    }
    /* dat_p will be unequal to zero if there is a valid 
     * http get */
    if(dat_p==0){
      // no http request
      continue;
    }
    // tcp port 80 begin
    if (strncmp("GET ",(char *)&(buf[dat_p]),4)!=0){
      // head, post and other methods:
      dat_p=http200ok();
      dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("<h1>200 OK</h1>"));
      goto SENDTCP;
    }
    
    // just one web page in the "root directory" of the web server
    if (strncmp("/ ",(char *)&(buf[dat_p+4]),2)==0){
      dat_p=print_webpage(buf);
      goto SENDTCP;
    } else if (strncmp("/saunaOnBtn/",(char *)&(buf[dat_p+4]),12)==0) {
      if (strncmp("0 ",   (char *)&(buf[dat_p+16]),2)==0 ) {
        saunaOnButtonActive = false;
      } else if (strncmp("1 ",(char *)&(buf[dat_p+16]),2)==0 ) {
        saunaOnButtonActive = true;
      } else if (strncmp("status ",(char *)&(buf[dat_p+16]),7)==0 ) {
        
      } else {
        dat_p=invalid_path_response(buf);
        goto SENDTCP;
      }
      dat_p=http200ok();
      if (saunaOnButtonActive) {
        dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("Sauna ON button is: Active\n"));
      } else {
        dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("Sauna is ON button: Not active\n"));
      }
    } else if (strncmp("/input/",(char *)&(buf[dat_p+4]),7)==0) {
      // Reading inputs 1-4
      if (strncmp("1 ",(char *)&(buf[dat_p+11]),2)==0 || strncmp("2 ",(char *)&(buf[dat_p+11]),2)==0 
       || strncmp("3 ",(char *)&(buf[dat_p+11]),2)==0 || strncmp("4 ",(char *)&(buf[dat_p+11]),2)==0) {
        char inputNumber[1] = {0};
        inputNumber[0] = buf[dat_p+11];
        int inputReading = analogRead(54 - inputNumber[0]);
        char charInputNumber[1];
        int input = inputNumber[0] - 48;
        itoa(input, charInputNumber, 6);
        dat_p=http200ok();
        dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("Input number "));
        dat_p=es.ES_fill_tcp_data(buf,dat_p, charInputNumber);
        if (inputReading > inputThreshold){
          dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR(" is ON"));
        } else {
          dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR(" is OFF"));
        }
        dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("\n"));
       } else { 
         dat_p=invalid_path_response(buf);
         goto SENDTCP;
       }
    } else if (strncmp("/relay/",(char *)&(buf[dat_p+4]),7)==0) { 
      // Reading and changing states of the relays
      if (strncmp("1",(char *)&(buf[dat_p+11]),1)==0 || strncmp("2",(char *)&(buf[dat_p+11]),1)==0 
       || strncmp("3",(char *)&(buf[dat_p+11]),1)==0 || strncmp("4",(char *)&(buf[dat_p+11]),1)==0) {
        char relayNumber[1] = {0};
        relayNumber[0] = buf[dat_p+11];
        if (strncmp("/status ",(char *)&(buf[dat_p+12]),8)==0) {
            // Read status of relays
          dat_p=http200ok();
          // 57 because the ASCII value of e.g. '1' is 49. The digital pins are 9 - the relay number, i.e. RELAY1 is 8 (57-49=8)
          if ( bitRead(PORTD, 57 - relayNumber[0]) ) {
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("Relay "));
            dat_p=es.ES_fill_tcp_data(buf,dat_p, relayNumber);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR(" state is: ON\n"));
          } else {
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("Relay "));
            dat_p=es.ES_fill_tcp_data(buf,dat_p,relayNumber);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR(" state is: OFF\n"));
          }
          goto SENDTCP;
        } else if (strncmp("/0 ",(char *)&(buf[dat_p+12]),3)==0 ) {
          dat_p=http200ok();
          if ( bitRead(PORTD, 57 - relayNumber[0]) ) {
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("Relay "));
            dat_p=es.ES_fill_tcp_data(buf,dat_p,relayNumber);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR(" state changed to: OFF\n"));
          } else {
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("Relay "));
            dat_p=es.ES_fill_tcp_data(buf,dat_p,relayNumber);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR(" state was already: OFF\n"));
          }
          digitalWrite(57 - relayNumber[0],LOW);
          goto SENDTCP;
        } else if (strncmp("/1 ",(char *)&(buf[dat_p+12]),3)==0 ) {
          dat_p=http200ok();
          if ( bitRead(PORTD, 57 - relayNumber[0]) ) {
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("Relay "));
            dat_p=es.ES_fill_tcp_data(buf,dat_p,relayNumber);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR(" state was already: ON\n"));
          } else {
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("Relay "));
            dat_p=es.ES_fill_tcp_data(buf,dat_p,relayNumber);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR(" state changed to: ON\n"));
          } 
          digitalWrite(57 - relayNumber[0],HIGH);
          goto SENDTCP;
        } else {
          dat_p=invalid_path_response(buf);
          goto SENDTCP;
        }
      } else {
        dat_p=invalid_path_response(buf);
        goto SENDTCP;
      }
    } else if (strncmp("/el/",(char *)&(buf[dat_p+4]),4)==0) {
      // /el/ electricity meter path
        int pos = dat_p + 8;
        // Figure out the meter/modbus ID
        sAddr = "";
        for (int i=pos; i<dat_p+12; i++) {
          pos++;
          if (strncmp("/",(char *)&(buf[i]),1)==0) {
            pathOk = true;
            break;
          } 
          if (strncmp(" ",(char *)&(buf[i]),1)==0) {
            pathOk = false;
            break;
          }
          sAddr += ((char *)&(buf[i]))[0];
        }
        if  ( !pathOk ) {
          dat_p=invalid_path_response(buf);
          goto SENDTCP;
        } else {
          mAddr = sAddr.toInt();
          // All good until now. Next part of the path to be parsed
          pathOk = false;
          // Figure out what type of info is requested for this meter 
  
          sReqType = "";
          for (int i=pos; i<pos+16; i++) {
            if (strncmp("/",(char *)&(buf[i]),1)==0 || strncmp(" ",(char *)&(buf[i]),1)==0) {
              pathOk = true;
              break;
            } 
            sReqType += ((char *)&(buf[i]))[0];
          }
        }
        if ( !pathOk ) {
          // We could not determine the path (
          dat_p=invalid_path_response(buf);
          goto SENDTCP;
        } 
        
        // Determine what is requested
        memset(reqType, 0, sizeof(reqType));
        sReqType.toCharArray(reqType, 16);
        if ( sReqType == "human1" ) {
          // Single phase meter, most important readings
          modbusResult = 0;
          float TotkWh, L1W, L1V, L1A ;
          readReq[0] = mAddr;
          // kWh
          readReq[2] = mReg[65][1];
          readReq[3] = mReg[65][2];
          modbusResult += sendModbusPacket();
          TotkWh = meterVal;
          // Power
          readReq[2] = mReg[6][1];
          readReq[3] = mReg[6][2];
          modbusResult += sendModbusPacket();
          L1W = meterVal;
          // Voltage 
          readReq[2] = mReg[0][1];
          readReq[3] = mReg[0][2];
          modbusResult += sendModbusPacket();
          L1V = meterVal;
          // Current
          readReq[2] = mReg[3][1];
          readReq[3] = mReg[3][2];
          modbusResult += sendModbusPacket();
          L1A = meterVal;
          char tempBuf[10] = {0};
          if ( modbusResult == 0 ) {
            dat_p=http200ok();
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("Modbus address: "));
            memset(tempBuf, 0, sizeof(tempBuf));
            itoa(readReq[0], tempBuf, 10);
            dat_p=es.ES_fill_tcp_data_len(buf,dat_p,tempBuf, 10);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("\n<br>"));
            // kWh
            dat_p=es.ES_fill_tcp_data(buf,dat_p,mReg[65][3]);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("<b>"));
            dtostrf(TotkWh, 10, 2, tempBuf);
            dat_p=es.ES_fill_tcp_data_len(buf,dat_p,tempBuf,10);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR(" "));
            dat_p=es.ES_fill_tcp_data(buf,dat_p,mReg[65][4]);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("</b>\n<br>"));
            // Power
            dat_p=es.ES_fill_tcp_data(buf,dat_p,mReg[6][3]);
            memset(tempBuf, 0, sizeof(tempBuf));
            dtostrf(L1W, 10, 2, tempBuf);
            dat_p=es.ES_fill_tcp_data_len(buf,dat_p,tempBuf,10);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR(" "));
            dat_p=es.ES_fill_tcp_data(buf,dat_p,mReg[6][4]);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("\n<br>"));
            // Voltage
            dat_p=es.ES_fill_tcp_data(buf,dat_p,mReg[0][3]);
            memset(tempBuf, 0, sizeof(tempBuf));
            dtostrf(L1V, 10, 2, tempBuf);
            dat_p=es.ES_fill_tcp_data_len(buf,dat_p,tempBuf,10);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR(" "));
            dat_p=es.ES_fill_tcp_data(buf,dat_p,mReg[0][4]);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("\n<br>"));
            // Current
            dat_p=es.ES_fill_tcp_data(buf,dat_p,mReg[3][3]);
            memset(tempBuf, 0, sizeof(tempBuf));
            dtostrf(L1A, 10, 2, tempBuf);
            dat_p=es.ES_fill_tcp_data_len(buf,dat_p,tempBuf,10);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR(" "));
            dat_p=es.ES_fill_tcp_data(buf,dat_p,mReg[3][4]);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("\n<br>"));
            goto SENDTCP;
          } else { 
            dat_p=http404();
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("Something went wrong handling your request.<br>\n"));
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("modbusResult (sum):"));
            memset(tempBuf, 0, sizeof(tempBuf));
            itoa(modbusResult, tempBuf, 10);
            // FIXME: this does not work as intended somehow..
            dat_p=es.ES_fill_tcp_data_len(buf,dat_p,tempBuf, 10);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("\n"));
            goto SENDTCP;
          }
        } else if ( sReqType == "human3" ) {
          // Three phase meter, most important readings
          modbusResult = 0;
          float TotkWh, TotW, AvgV, Totpf, L1A, L2A, L3A ;
          readReq[0] = mAddr;
          // kWh
          readReq[2] = mReg[65][1];
          readReq[3] = mReg[65][2];
          modbusResult += sendModbusPacket();
          TotkWh = meterVal;
          // Total power
          readReq[2] = mReg[24][1];
          readReq[3] = mReg[24][2];
          modbusResult += sendModbusPacket();
          TotW = meterVal;
          // Avg. voltage 
          readReq[2] = mReg[21][1];
          readReq[3] = mReg[21][2];
          modbusResult += sendModbusPacket();
          AvgV = meterVal;
          // Tot. pf
          readReq[2] = mReg[27][1];
          readReq[3] = mReg[27][2];
          modbusResult += sendModbusPacket();
          Totpf = meterVal;
          // Current
          readReq[2] = mReg[3][1];
          readReq[3] = mReg[3][2];
          modbusResult += sendModbusPacket();
          L1A = meterVal;
          readReq[2] = mReg[4][1];
          readReq[3] = mReg[4][2];
          modbusResult += sendModbusPacket();
          L2A = meterVal;
          readReq[2] = mReg[5][1];
          readReq[3] = mReg[5][2];
          modbusResult += sendModbusPacket();
          L3A = meterVal;
          char tempBuf[10] = {0};
          if ( modbusResult == 0 ) {
            dat_p=http200ok();
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("Modbus address: "));
            memset(tempBuf, 0, sizeof(tempBuf));
            itoa(readReq[0], tempBuf, 10);
            dat_p=es.ES_fill_tcp_data_len(buf,dat_p,tempBuf, 10);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("\n<br>"));
            // kWh
            dat_p=es.ES_fill_tcp_data(buf,dat_p,mReg[65][3]);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("<b>"));
            memset(tempBuf, 0, sizeof(tempBuf));
            dtostrf(TotkWh, 10, 2, tempBuf);
            dat_p=es.ES_fill_tcp_data_len(buf,dat_p,tempBuf,10);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR(" "));
            dat_p=es.ES_fill_tcp_data(buf,dat_p,mReg[65][4]);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("</b>\n<br>"));
            // Total power
            dat_p=es.ES_fill_tcp_data(buf,dat_p,mReg[24][3]);
            memset(tempBuf, 0, sizeof(tempBuf));
            dtostrf(TotW, 10, 2, tempBuf);
            dat_p=es.ES_fill_tcp_data_len(buf,dat_p,tempBuf,10);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR(" "));
            dat_p=es.ES_fill_tcp_data(buf,dat_p,mReg[24][4]);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("\n<br>"));
            // Avg. voltage
            dat_p=es.ES_fill_tcp_data(buf,dat_p,mReg[21][3]);
            memset(tempBuf, 0, sizeof(tempBuf));
            dtostrf(AvgV, 10, 2, tempBuf);
            dat_p=es.ES_fill_tcp_data_len(buf,dat_p,tempBuf,10);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR(" "));
            dat_p=es.ES_fill_tcp_data(buf,dat_p,mReg[21][4]);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("\n<br>"));
            // Total power factor
            dat_p=es.ES_fill_tcp_data(buf,dat_p,mReg[27][3]);
            memset(tempBuf, 0, sizeof(tempBuf));
            dtostrf(Totpf, 10, 2, tempBuf);
            dat_p=es.ES_fill_tcp_data_len(buf,dat_p,tempBuf,10);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR(" "));
            dat_p=es.ES_fill_tcp_data(buf,dat_p,mReg[27][4]);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("\n<br>"));
            // Current
            dat_p=es.ES_fill_tcp_data(buf,dat_p,mReg[3][3]);
            memset(tempBuf, 0, sizeof(tempBuf));
            dtostrf(L1A, 10, 2, tempBuf);
            dat_p=es.ES_fill_tcp_data_len(buf,dat_p,tempBuf,10);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR(" "));
            dat_p=es.ES_fill_tcp_data(buf,dat_p,mReg[3][4]);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("\n<br>"));
            dat_p=es.ES_fill_tcp_data(buf,dat_p,mReg[4][3]);
            memset(tempBuf, 0, sizeof(tempBuf));
            dtostrf(L2A, 10, 2, tempBuf);
            dat_p=es.ES_fill_tcp_data_len(buf,dat_p,tempBuf,10);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR(" "));
            dat_p=es.ES_fill_tcp_data(buf,dat_p,mReg[4][4]);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("\n<br>"));
            dat_p=es.ES_fill_tcp_data(buf,dat_p,mReg[5][3]);
            memset(tempBuf, 0, sizeof(tempBuf));
            dtostrf(L3A, 10, 2, tempBuf);
            dat_p=es.ES_fill_tcp_data_len(buf,dat_p,tempBuf,10);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR(" "));
            dat_p=es.ES_fill_tcp_data(buf,dat_p,mReg[5][4]);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("\n<br>"));
            goto SENDTCP;
          } else { 
            dat_p=http404();
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("Something went wrong handling your request.<br>\n"));
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("modbusResult (sum):"));
            memset(tempBuf, 0, sizeof(tempBuf));
            itoa(modbusResult, tempBuf, 10);
            // FIXME: this does not work as intended somehow..
            dat_p=es.ES_fill_tcp_data_len(buf,dat_p,tempBuf, 10);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("\n"));
            goto SENDTCP;
          }
        } else if ( sReqType == "kwh" ) {
          // Three phase meter, most important readings
          modbusResult = 0;
          float TotkWh;
          readReq[0] = mAddr;
          // kWh
          readReq[2] = mReg[65][1];
          readReq[3] = mReg[65][2];
          modbusResult += sendModbusPacket();
          TotkWh = meterVal;
          char tempBuf[10] = {0};
          if ( modbusResult == 0 ) {
            dat_p=http200ok();
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("Modbus address: "));
            memset(tempBuf, 0, sizeof(tempBuf));
            itoa(readReq[0], tempBuf, 10);
            dat_p=es.ES_fill_tcp_data_len(buf,dat_p,tempBuf, 10);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("\n<br>"));
            // kWh
            dat_p=es.ES_fill_tcp_data(buf,dat_p,mReg[65][3]);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("<b>"));
            memset(tempBuf, 0, sizeof(tempBuf));
            dtostrf(TotkWh, 10, 2, tempBuf);
            dat_p=es.ES_fill_tcp_data_len(buf,dat_p,tempBuf,10);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR(" "));
            dat_p=es.ES_fill_tcp_data(buf,dat_p,mReg[65][4]);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("</b>\n<br>"));
            goto SENDTCP;
          } else { 
            dat_p=http404();
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("Something went wrong handling your request.<br>\n"));
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("modbusResult (sum):"));
            memset(tempBuf, 0, sizeof(tempBuf));
            itoa(modbusResult, tempBuf, 10);
            // FIXME: this does not work as intended somehow..
            dat_p=es.ES_fill_tcp_data_len(buf,dat_p,tempBuf, 10);
            dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("\n"));
            goto SENDTCP;
          }
        }
    }
    else{
      dat_p=invalid_path_response(buf);
      goto SENDTCP;
    }
  SENDTCP:
    es.ES_www_server_reply(buf,dat_p); // send web page data
    // tcp port 80 end
  }
}
