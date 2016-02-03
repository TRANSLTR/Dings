// RFM12B Sender for TMP36 Sensor
//
// Basiert zum Teil auf der Arbeit von Nathan Chantrell
//
// modified by meigrafd @ 16.12.2013
//------------------------------------------------------------------------------
#include <RFM12B.h>
#include <avr/sleep.h>
//------------------------------------------------------------------------------
// You will need to initialize the radio by telling it what ID it has and what network it's on
// The NodeID takes values from 1-127, 0 is reserved for sending broadcast messages (send to all nodes)
// The Network ID takes values from 0-255
// By default the SPI-SS line used is D10 on Atmega328. You can change it by calling .SetCS(pin) where pin can be {8,9,10}
#define NODEID         19  // network ID used for this unit
#define NETWORKID     210  // the network ID we are on
#define GATEWAYID      22  // the node ID we're sending to
#define ACK_TIME     2000  // # of ms to wait for an ack
#define SENDDELAY   15000  // wait this many ms between sending packets
#define requestACK  true  // request ACK? (true/false)
//------------------------------------------------------------------------------
// PIN-Konfiguration 
//------------------------------------------------------------------------------
// SENSOR pins
#define tempPin 10       // Sensor Vout connected to D10 (ATtiny pin 13)
#define tempPower 9      // Sensor Power connected to D9 (ATtiny pin 12)
// LED pin
#define LEDpin 8         // LED power connected to D8 (ATtiny pin 11)
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

//encryption is OPTIONAL
//to enable encryption you will need to:
// - provide a 16-byte encryption KEY (same on all nodes that talk encrypted)
// - to call .Encrypt(KEY) to start encrypting
// - to stop encrypting call .Encrypt(NULL)
//#define KEY   "ABCDABCDABCDABCD"

// Need an instance of the Radio Module
RFM12B radio;

// Analogue reading from the sensor
float tempReading;

// String zum Versand
char msg[26];

//##############################################################################

static void activityLed (byte state) {
  pinMode(LEDpin, OUTPUT);
  digitalWrite(LEDpin, state);
}

// blink led
static void blink (byte pin, byte n = 3) {
  pinMode(pin, OUTPUT);
  for (byte i = 0; i < 2 * n; ++i) {
    delay(100);
    digitalWrite(pin, !digitalRead(pin));
  }
}

//--------------------------------------------------------------------------------------------------
// Read current supply voltage (in mV)
//--------------------------------------------------------------------------------------------------
long readVcc() {
  bitClear(PRR, PRADC);
  ADCSRA |= bit(ADEN); // Enable the ADC
  long result;
  // Read 1.1V reference against Vcc
  #if defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0); // For ATtiny84
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1); // For ATmega328
  #endif
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate Vcc in mV
  ADCSRA &= ~ bit(ADEN);
  bitSet(PRR, PRADC); // Disable the ADC to save power
  return result;
}

// getVoltage() - returns the voltage on the analog input defined by pin
float getVoltage(int pin) {
  //converting from a 0 to 1023 digital range to 0 to 5 volts (each 1 reading equals ~ 5 millivolts)
  return (analogRead(pin) * .004882814);
}

// setup() - this function runs once when you turn your ATtiny on
void setup() {
//  analogReference(INTERNAL);   // Set the aref to the internal 1.1V reference
  pinMode(tempPower, OUTPUT);
  pinMode(tempPin, INPUT);
  radio.Initialize(NODEID, RF12_433MHZ, NETWORKID);
  #ifdef KEY
    radio.Encrypt((byte*)KEY);      //comment this out to disable encryption
  #endif
  // configure RFM12B
  radio.Control(0xC040);   // Adjust low battery voltage to 2.2V
  radio.Sleep(); //sleep right away to save power
  PRR = bit(PRTIM1); // only keep timer 0 going
  ADCSRA &= ~ bit(ADEN); bitSet(PRR, PRADC); // Disable the ADC to save power
  if (LEDpin) {
    activityLed(1); // LED on
    delay(1000);
    activityLed(0); // LED off
  }
}

// loop() - run over and over again
void loop() {
  pinMode(tempPower, OUTPUT);
  digitalWrite(tempPower, HIGH); // turn Sensor on

  delay(1000);  // Kurzer Delay zur Initialisierung (DHT22 benoetigt mindestens 2 Sekunden fuer Aufwaermphase nach dem Power-On)

  if (LEDpin) {
    blink(LEDpin, 1); // blink LED
  }

  bitClear(PRR, PRADC); // power up the ADC
  ADCSRA |= bit(ADEN);  // Enable the ADC
  analogRead(tempPin); // throw away the first reading
  ADCSRA &= ~ bit(ADEN); // disable the ADC
  bitSet(PRR, PRADC); // power down the ADC

  int supplymV = readVcc(); // Get supply voltage
  float supplyV = supplymV / 1000.0;
  float aref = analogRead(tempPin) * supplyV;
  aref /= 1024.0;  // reference volts
  aref = aref * 1000; 
  analogReference(aref);  // reference millivolts
  delay(500);

  for(int i = 0; i < 10 ; i++) { // take 10 more readings
    tempReading += analogRead(tempPin); //getting the voltage reading from the temperature sensor
    delay(10);  // Short delay to slow things down
  }
  tempReading = tempReading / 10 ; // calculate the average

  float voltage = tempReading * aref;
  int temp = abs(voltage - 500) / 10; // Convert to temperature in degrees C.

/*
  double voltage = tempReading * (1230/1024); // Convert to mV (assume internal reference is accurate)
//  double voltage = tempReading * 0.942382812; // Calibrated conversion to mV
  double temperatureC = (voltage - 500) / 10; // Convert to temperature in degrees C. 
  int temp = temperatureC * 100; // Convert temperature to an integer, reversed at receiving end
*/
/*
// float tempReading = getVoltage(tempPin); //getting the voltage reading from the temperature sensor
 int temp = (tempReading - .5) * 100;   //converting from 10 mv per degree wit 500 mV offset
 */                                          //to degrees ((voltage - 500mV) times 100)

  // msg-Variable mit Daten zum Versand füllen, die später an das WebScript übergeben werden
  snprintf(msg, 26, "v=%d&t=%d", supplymV,temp) ;

  radio.Wakeup();
  radio.Send(GATEWAYID, (uint8_t *)msg, strlen(msg), requestACK);
  radio.SendWait(0);    //wait for RF to finish sending (2=standby mode)

  radio.Sleep();
  digitalWrite(tempPower, LOW); // turn Sensor off to save power
  pinMode(tempPower, INPUT); // set power pin for Sensor to input before sleeping, saves power

  ADCSRA &= ~ bit(ADEN); // disable the ADC
  bitSet(PRR, PRADC); // power down the ADC

  if (LEDpin) {
    blink(LEDpin, 2); // blink LED
  }
  delay(SENDDELAY);
}

