#include <JeeLib.h> // https://github.com/jcw/jeelib
#include "pins_arduino.h"
#include <avr/sleep.h>
#include <Ports.h>


#define MAX_V_BAT 13800   // Ladeschlussspannung
#define MIN_V_BAT 12500   // Spannung bei der die Verbraucher abgeschaltet werden
#define LOW_V_BAT 13000   // Spannung bei der die Verbraucher wieder eingeschaltet werden

#define SEND_DELAY 1000   // Sample Time
#define DWN_SAMPLE 100    // in jedem wievielten Takt soll gesendet werden?
#define DUNKEL 1980       // Helligkeitswert, ab dem das Licht angeschaltet werden soll
#define HELL 1500
#define STUNDEN 5         // Leuchtdauer in Stunden
#define LEUCHTDAUER SEND_DELAY/10*36*STUNDEN  // Zeit, die das Licht anbleiben soll

#define LADEN 10          // Batterie laden
#define MOTOR 9           // Motor ein
#define N_LOW_BAT 0         // Verbraucher aus
#define LEDpin 3          // Status-LED-Pin 

#define NODEID 01       // RF12 node ID in the range 1-30
#define NETWORKID 210       // RF12 Network group
#define myUID 12          // device id

#define FREQ RF12_433MHZ  // Frequency of RFM12B module

//#define USE_ACK           // Enable ACKs, comment out to disable
#define RETRY_PERIOD 3    // How soon to retry (in seconds) if ACK didn't come in
#define RETRY_LIMIT 5     // Maximum number of times to retry
#define ACK_TIME 22       // Number of milliseconds to wait for an ack
#define DATA_RATE_49200  0xC606           // Approx  49200 bps
#define DATA_RATE_38400  0xC608           //  Approx  38400 bps
#define DATA_RATE        DATA_RATE_38400  // use one of the data rates defined above

#define V_BAT_ADC 3       // ADC Number for battery voltage measurment
#define V_LIGHT_ADC 2     // ADC Number for brightness measurment

#define magicNumber 83    // magic number (GSD-Net identifier) => SubNodeID 1 Byte




/*
                     +-\/-+
               VCC  1|    |14  GND
          (D0) PB0  2|    |13  AREF (D10)
          (D1) PB1  3|    |12  PA1 (D9)
             RESET  4|    |11  PA2 (D8)
INT0  PWM (D2) PB2  5|    |10  PA3 (D7)
      PWM (D3) PA7  6|    |9   PA4 (D6)
      PWM (D4) PA6  7|    |8   PA5 (D5) PWM
                     +----+
*/

ISR(WDT_vect) {}

/* Fuer direkt lesbare Uebertragung
typedef struct {
   byte magic;   // magic number
   byte uID;     // deviceID   
   int msg_counter;
  
   byte m_powerSupply;    // 202
   byte powerSupply;      // Supply voltage
   byte powerSupply_Komma;
   byte m_Light;          // 200
   byte Light;            // Brightness level
   byte Light_Komma;      // Brightness level
   int Brenndauer;
   int Brenndauer_max;
   byte Ladestatus;
   byte Batteriestatus; 
   byte Lichtstatus; 
   
 } Payload;
 */
 // Fuer FHEM
 typedef struct {
   byte magic;   // magic number
   byte uID;     // deviceID   
   int msg_counter;
  
   byte m_powerSupply;    // 202
   int powerSupply;      // Supply voltage
   
   byte m_Light;          // 200
   int Light;            // Brightness level
   
   byte m_Brenndauer;          // 201
   int Brenndauer;
   
   byte m_Ladestatus;          // 197
   byte Ladestatus;
   
   byte m_Batteriestatus;          // 198
   byte Batteriestatus; 
   
   byte m_Lichtstatus;          // 199
   byte Lichtstatus; 
   
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
 
 long V_Bat;
 long V_Light;
 char Status;
 long Dauer;
 char  ctr;
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

 
 // blink led
static void blink (byte pin, byte n = 3) {
  if (LEDpin) {
    pinMode(pin, OUTPUT);
    for (byte i = 0; i < 2 * n; ++i) {
      delay(100);
      digitalWrite(pin, !digitalRead(pin));
    }
  }
}

long read_ADC5(char Nummer){
  bitClear(PRR, PRADC);
  ADCSRA |= bit(ADEN); // Enable the ADC
  long result;
  // Read 1.1V reference against Vcc
  #if defined(__AVR_ATtiny84__)
  if(Nummer>0 && Nummer <8){
    //ADMUX = _BV(MUX5) | _BV(MUX0); // For ATtiny84 meas. against int. ref.
    ADMUX = Nummer; // For ATtiny84, ADC0-7
  }
  else
  {
     ADMUX =2;
  }
  #else
    //ADMUX = _BV(REFS0) | _BV(MUX2) | _BV(MUX0); // For ATmega328 - int. Ref AVcc - Ch.5
  #endif
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = result*5000/1024; // Back-calculate Vcc in mV
  //ADCSRA &= ~ bit(ADEN);
  //bitSet(PRR, PRADC); // Disable the ADC to save power
  return result;
}


 void setup() {
   //blink(LEDpin, 2);
   
     tinytx.magic             = magicNumber;
     tinytx.msg_counter       = 1;
     tinytx.uID               = myUID;             
     tinytx.m_powerSupply     = 202;    
     tinytx.m_Light           = 200;     
     tinytx.m_Brenndauer      = 201;          
     tinytx.m_Ladestatus      = 197;      
     tinytx.m_Batteriestatus  = 198;         
     tinytx.m_Lichtstatus     = 199;         
               
               
     pinMode(LEDpin, OUTPUT);
     pinMode(LADEN, OUTPUT);
     pinMode(MOTOR, OUTPUT);
     pinMode(N_LOW_BAT, OUTPUT);
     
     digitalWrite(LEDpin, LOW);
     digitalWrite(LADEN, LOW);
     digitalWrite(MOTOR, LOW);
     digitalWrite(N_LOW_BAT, LOW);
     
     rf12_initialize(NODEID, FREQ, NETWORKID); // Initialize RFM12 with settings defined above
 #ifdef KEY
  //radio.Encrypt((byte*)KEY);
    rf12_encrypt((byte*)KEY);
 #endif
 rf12_control(0xC040);      // Adjust low battery voltage to 2.2V
  rf12_sleep(0);              // Put the RFM12 to sleep
  
      Status = 0;
      Dauer = 0; // Zaehlt mit jedem Sendevorgang hoch
      ctr = 0;
}

void loop() {
  
   V_Bat = read_ADC5(V_BAT_ADC);
   V_Bat = V_Bat*3.684;
   
   tinytx.powerSupply =  V_Bat;
   //tinytx.powerSupply =  V_Bat/1000;
   //tinytx.powerSupply_Komma =  (V_Bat-tinytx.powerSupply*1000)/10;
   V_Light = read_ADC5(V_LIGHT_ADC);
   tinytx.Light = V_Light;
   //tinytx.Light = V_Light/1000;
   //tinytx.Light_Komma =  (V_Light-tinytx.Light*1000)/10;
   if(MAX_V_BAT > V_Bat){
      digitalWrite(LADEN, HIGH);
      tinytx.Ladestatus = 1;
     
   }   
   else{
     digitalWrite(LADEN, LOW);
     tinytx.Ladestatus = 0;
     
   }
   
   
   
   if(LOW_V_BAT < V_Bat){
     Status = 1;
     
   }
   if(MIN_V_BAT > V_Bat){
     Status = 0;
     
   }
   
   if(MIN_V_BAT < V_Bat){
          
     if((V_Light>DUNKEL)&&(Status==1)&&(Dauer < LEUCHTDAUER)){ // ist dunkel und Batterie erlaubt einschalten und Leuchtdauer nicht ueberschritten
       digitalWrite(N_LOW_BAT, HIGH);
       digitalWrite(LEDpin, HIGH);
       tinytx.Lichtstatus = 1;
     }
     else{
       digitalWrite(N_LOW_BAT, LOW);
       digitalWrite(LEDpin, LOW);
       tinytx.Lichtstatus = 0;
     }     
   }
   else{
       digitalWrite(N_LOW_BAT, LOW);
       digitalWrite(LEDpin, LOW);
        Status = 0;
        tinytx.Lichtstatus = 0;
   }
   if(V_Light< HELL){     
     digitalWrite(N_LOW_BAT, LOW);
     digitalWrite(LEDpin, LOW);
   }
   ctr++;
   if (ctr >= DWN_SAMPLE){
      tinytx.msg_counter++;
      tinytx.Brenndauer = LEUCHTDAUER-Dauer;
      tinytx.Batteriestatus = Status;
     
      rfwrite();
   ctr = 0;
   }
     if(V_Light< HELL){     
       Dauer = 0;
     }
     if(V_Light>DUNKEL){
       if(Dauer <= LEUCHTDAUER){
         Dauer++;
       }
     }
   
   
  //blink(LEDpin, 2); // blink LED
  Sleepy::loseSomeTime(SEND_DELAY); //JeeLabs power save function: enter low power mode for 60 seconds (valid range 16-65000 ms)
//delay(5000);
}
