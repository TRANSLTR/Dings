
#include <JeeLib.h> // https://github.com/jcw/jeelib
#include "pins_arduino.h"
#include <avr/sleep.h>
#include <Ports.h>

/*
// ATmega168, ATmega328, etc.
#define RFM_IRQ     2
#define SS_DDR      DDRB
#define SS_PORT     PORTB
#define SS_BIT      2     // for PORTB: 2 = d.10, 1 = d.9, 0 = d.8

#define SPI_SS      10    // PB2, pin 16
#define SPI_MOSI    11    // PB3, pin 17
#define SPI_MISO    12    // PB4, pin 18
#define SPI_SCK     13    // PB5, pin 19
*/

#define NODEID 01       // RF12 node ID in the range 1-30
#define NETWORKID 210       // RF12 Network group
#define myUID 55          // device id

#define FREQ RF12_433MHZ  // Frequency of RFM12B module

//#define USE_ACK           // Enable ACKs, comment out to disable
#define RETRY_PERIOD 3    // How soon to retry (in seconds) if ACK didn't come in
#define RETRY_LIMIT 5     // Maximum number of times to retry
#define ACK_TIME 22       // Number of milliseconds to wait for an ack
#define SEND_DELAY 1000   // Sample Time
#define DWN_SAMPLE 100       // in jedem wievielten Takt soll gesendet werden?
#define DATA_RATE_49200  0xC606           // Approx  49200 bps
#define DATA_RATE_38400  0xC608           //  Approx  38400 bps
#define DATA_RATE        DATA_RATE_38400  // use one of the data rates defined above

#define V_BAT_ADC 3       // ADC Number for battery voltage measurment
#define V_LIGHT_ADC 2     // ADC Number for brightness measurment

#define magicNumber 83    // magic number (GSD-Net identifier) => SubNodeID 1 Byte
ISR(WDT_vect) {}


typedef struct {
   byte magic;   // magic number
   byte uID;     // deviceID   
   int msg_counter;  
   byte m_powerSupply;    // 202
   int powerSupply;      // Supply voltage   
 } Payload;

 Payload tinytx;

// Wait a few milliseconds for proper ACK
 #ifdef USE_ACK
  static byte waitForAck() {
   MilliTimer ackTimer;
   while (!ackTimer.poll(ACK_TIME)) {
     if (rf12_recvDone() && rf12_crc == 0 &&
        rf12_hdr == (RF12_HDR_DST | RF12_HDR_CTL | NODEID)) {
          if ((rf12_len > 0 && rf12_data[0] == myUID) || (rf12_len == 0)) {
            return 1;
          }
        }
        //set_sleep_mode(SLEEP_MODE_IDLE);
        //sleep_mode();
     }
   return 0;
  }
 #endif
 
 //--------------------------------------------------------------------------------------------------
// Send payload data via RF
//-------------------------------------------------------------------------------------------------
 static byte rfwrite(){
  #ifdef USE_ACK
   for (byte i = 0; i <= RETRY_LIMIT; ++i) {  // tx and wait for ack up to RETRY_LIMIT times
     rf12_sleep(-1);              // Wake up RF module
      while (!rf12_canSend())
      rf12_recvDone();
      rf12_sendStart(RF12_HDR_ACK,&tinytx, sizeof(tinytx)); 
      rf12_sendWait(2);           // Wait for RF to finish sending while in standby mode
      byte acked = waitForAck();  // Wait for ACK
      rf12_sleep(0);              // Put RF module to sleep
      if (acked) { return 1; }      // Return if ACK received
  
   Sleepy::loseSomeTime(RETRY_PERIOD * 1000);     // If no ack received wait and try again
   }
   return 0;
  #else
     rf12_sleep(-1);              // Wake up RF module
     while (!rf12_canSend())
     rf12_recvDone();
     rf12_sendStart(0, &tinytx, sizeof(tinytx)); 
     rf12_sendWait(2);           // Wait for RF to finish sending while in standby mode
     rf12_sleep(0);              // Put RF module to sleep
     return 1;
  #endif
 }
 
 void setup() {
    tinytx.magic             = magicNumber;
     tinytx.msg_counter       = 1;
     tinytx.uID               = myUID;  
      rf12_initialize(NODEID, FREQ, NETWORKID); // Initialize RFM12 with settings defined above
     rf12_control(0xC040);      // Adjust low battery voltage to 2.2V
  rf12_sleep(0);              // Put the RFM12 to sleep
  
}

void loop() {
  
  tinytx.powerSupply =  1;
  
   tinytx.msg_counter++;
    rfwrite();
    
    Sleepy::loseSomeTime(SEND_DELAY); //JeeLabs power save function: enter low power mode for 60 seconds (valid range 16-65000 ms)

}
