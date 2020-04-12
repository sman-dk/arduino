/* 
 *  Note2self: Parts of this code is a bit messy. Loats of globals and even a goto.. but hey it works.
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

uint16_t plen, dat_p; // Web server stuff

// s0 timers etc.
const unsigned int s0_min_time = 50;
const unsigned int s0_max_time = 3000;
unsigned long long input1_time = 0;
unsigned long long input2_time = 0;
int inputThreshold = 600;
unsigned long long input1_counter = 0;
unsigned long long input2_counter = 0;
boolean input1_s0_signal = false;
boolean input2_s0_signal = false;

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

uint16_t invalid_path_response(uint8_t *buf)
{
  uint16_t plen;
  plen=http404();
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("Invalid path\n"));
  return(plen);
}

void setup()
{
 // Watchdog
 wdt_enable(WDTO_8S);
 // Init serial communication
 Serial.begin(9600); 
 
 // Digital inputs setup
 pinMode(INPUT1, INPUT_PULLUP); // it seams INPUT_PULLUP has no effect on a KMTRONIC DINo board
 pinMode(INPUT2, INPUT_PULLUP);
 pinMode(INPUT3, INPUT_PULLUP);
 pinMode(INPUT4, INPUT_PULLUP);
 
 // Disable all outlets
 digitalWrite(RELAY1,0);
 digitalWrite(RELAY2,0);
 digitalWrite(RELAY3,0);
 digitalWrite(RELAY4,0);

  // initialize enc28j60
  es.ES_enc28j60Init(mymac);

  // init the ethernet/ip layer:
  es.ES_init_ip_arp_udp_tcp(mymac,myip, MYWWWPORT);

  Serial.println("VammenS0 started..");
}

void loop()
{
  wdt_reset(); // Watchdog
    // A "while true", in order to be able to do a "continue" a few lines below..
    while (1) {
    wdt_reset(); // Watchdog
    // read packet, handle ping and wait for a tcp packet:
    dat_p=es.ES_packetloop_icmp_tcp(buf,es.ES_enc28j60PacketReceive(BUFFER_SIZE, buf));
    // input1 counting
    if (analogRead(INPUT1) < inputThreshold ) {
      if ( !input1_s0_signal ) {
        input1_time = millis();
      }
      input1_s0_signal = true;
    } else {
      if (input1_s0_signal){
        unsigned long long deltaTime = millis() - input1_time;
        if (deltaTime > s0_min_time && deltaTime < s0_max_time) {
          input1_counter++;
        }
      }
      input1_s0_signal = false;
    }
    // input2 counting
    if (analogRead(INPUT2) < inputThreshold ) {
      if ( !input2_s0_signal ) {
        input2_time = millis();
      }
      input2_s0_signal = true;
    } else {
      if (input2_s0_signal){
        unsigned long long deltaTime = millis() - input2_time;
        if (deltaTime > s0_min_time && deltaTime < s0_max_time) {
          input2_counter++;
        }
      }
      input2_s0_signal = false;
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
     // dat_p=print_webpage(buf);
        char charCounter1[11];
        itoa(input1_counter, charCounter1, 10);
        dat_p=http200ok();
        dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("Counting s0 pulses just for you..\n"));
        dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("input1: "));
        dat_p=es.ES_fill_tcp_data(buf,dat_p, charCounter1);
        char charCounter2[11];
        itoa(input2_counter, charCounter2, 10);
        dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("\ninput2: "));
        dat_p=es.ES_fill_tcp_data(buf,dat_p, charCounter2);
        dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("\n"));
        dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("Uptime: "));
        char uptime[7] = {0, 0, 0, 0, 0, 0, 0};
        unsigned long mUp = millis()/1000;
        ltoa(mUp, uptime, 7);
        dat_p=es.ES_fill_tcp_data_len(buf,dat_p, uptime, 7);
        dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR(" s\n"));
        goto SENDTCP;
    } else { 
      // We could not determine the path (
      dat_p=invalid_path_response(buf);
      goto SENDTCP;
    } 
  SENDTCP:
    es.ES_www_server_reply(buf,dat_p); // send web page data
    // tcp port 80 end
  }
}
