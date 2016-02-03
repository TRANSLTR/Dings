/*

Pinbelegung ATtiny 84PU
D:\Ahr\Schleifen\Messaufbau\Temperaturmessung\Temperaturmessung2_0\TM2_1.sch

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
/* 
  The circuit:
 * LCD RS pin to digital pin 3
 * LCD Enable pin to digital pin 4
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 6
 * LCD D6 pin to digital pin 7
 * LCD D7 pin to digital pin 8
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)
 
*/

// include the library code:
#include <LiquidCrystal.h>
#include "pins_arduino.h"
#include <OneWire.h>
#include <DallasTemperature.h>


// Definitions

#define ONE_WIRE_BUS 10   // DS18B20 Temperature sensor is connected on D10/ATtiny pin 13
#define ONE_WIRE_POWER 9  // DS18B20 Power pin is connected on D9/ATtiny pin 12

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(3, 4, 5, 6, 7, 8);

// Setup a oneWire instance to communicate with any OneWire devices 
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
 
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);



void setup() {
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
    // set the cursor to column 0, line 0
    // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 0);
  // Print a message to the LCD.
  lcd.print("Pi ist genau 3!");
  delay(2000);
  lcd.clear();
}

void loop() {
 
 //Get temperature
 pinMode(ONE_WIRE_POWER, OUTPUT); // set power pin for DS18B20 to output
 digitalWrite(ONE_WIRE_POWER, HIGH); // turn DS18B20 sensor on
  delay(50); // Allow 50ms for the sensor to be ready

 // Start up the library
 sensors.begin();
 sensors.requestTemperatures(); // Send the command to get temperatures  

 int temp = sensors.getTempCByIndex(0)*100;

 digitalWrite(ONE_WIRE_POWER, LOW); // turn Sensor off to save power
 pinMode(ONE_WIRE_POWER, INPUT); // set power pin for Sensor to input before sleeping, saves power

  lcd.setCursor(0, 0);
  // Print a message to the LCD.
  lcd.print("Spule 1:");
  lcd.setCursor(0, 1);
  lcd.print((float)temp/(float)100);
  lcd.print((char)223); // Grad Zeichen
  lcd.print("C    ");
  delay(100);

}

