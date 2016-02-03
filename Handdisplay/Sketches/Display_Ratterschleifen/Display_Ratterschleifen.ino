
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>


#define TFT_CS     10
#define TFT_RST    9                     
#define TFT_DC     8

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);

#define TFT_SCLK 13   // Hardware SPI CLK
#define TFT_MOSI 11   // Hardware SPI MOSI

int Amp[22];
unsigned int input = 0;
char msg[10];
int supplyV =0;

void setup(void) {
  DDRD = 0x00;
  PORTD= 0xFF; 
  tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab
  
  tft.setRotation(1);
  tft.fillScreen(ST7735_WHITE);
  
  // tft print function!
  Achsenbeschriftung();
  
  for(int i=10;i<=32;i++){
  //Amp 0-100
    Amp[i-10] = (i-10)*4;
    Balken_aktualisieren(i,Amp[i-10]); 
  }
 
  delay(4000);

}

void loop() {
  input =255 - (unsigned int)PIND +1;
  
  supplyV=(read_ADC5()/100)*2;  

  
  if(supplyV>100)
   supplyV=100;
  if(supplyV<0)
  supplyV=0;
  
  if(input > 10) {
    if(input>32)
       input=32;
 
  
    Balken_aktualisieren(input,(int) supplyV);  
  }
}



  void Achsenbeschriftung() {
 
  int idx = 0;
  char txt[2];
  
  tft.setCursor(1, 4);
  // y-Achse
  tft.setTextWrap(false);
  tft.setTextColor(ST7735_BLACK);
  tft.setTextSize(1);
  tft.println(1);
   tft.setCursor(8, 4);
  tft.setTextWrap(false);
  tft.setTextColor(ST7735_BLACK);
  tft.setTextSize(1);
  tft.println(0);
  
for(idx=9;idx>=0;idx--){  
  itoa(idx,&txt[0],10);
  tft.setCursor(8, 4+10*(10-idx));
  tft.setTextWrap(false);
  tft.setTextColor(ST7735_BLACK);
  tft.setTextSize(1);
  tft.println(txt[0]);
  tft.drawFastHLine(15, 15+10*(8-idx), 2, ST7735_BLACK);
  tft.drawFastHLine(18, 15+10*(8-idx), 141, 0b1110111101011101);
  for(int i=10;i<=32;i++){
    tft.drawFastVLine(18+(i-10)*6, 5, 100, ST7735_WHITE);
    tft.drawFastVLine(19+(i-10)*6, 5, 100, ST7735_WHITE);  
    tft.drawFastVLine(20+(i-10)*6, 5, 100, ST7735_WHITE);
  }
  }
  
  //x-Achse
  for(idx=18;idx<41;idx=idx+2){  
  itoa(idx,&txt[0],10);
  tft.setCursor(15+(idx-18)*6, 108);
  tft.setTextWrap(false);
  tft.setTextColor(ST7735_BLACK);
  tft.setTextSize(1);
  tft.println(txt[0]);
  tft.setCursor(20+(idx-18)*6, 113);
  tft.println(txt[1]);
  tft.setCursor(70,121);
  tft.println("Wellen");
  tft.setRotation(0);
  tft.setCursor(40,0);
  tft.println("Amplitude");
  tft.setRotation(1);

  tft.drawFastVLine(19+(idx-10)*6, 106, 2, ST7735_BLACK);
  }
  //Achsen
  tft.drawFastHLine(17, 105, 139, ST7735_BLACK);
  tft.drawFastVLine(17, 4, 102, ST7735_BLACK);
  tft.fillTriangle(160, 105,156,108, 156, 102, ST7735_BLACK);
  tft.fillTriangle(14, 3,17,0, 20, 3, ST7735_BLACK);
  
}

void Balken_aktualisieren(int ii, int A) {
  if(A>=0){    
    tft.drawFastVLine(18+(ii-10)*6, 5, 100-A, ST7735_WHITE);
    tft.drawFastVLine(19+(ii-10)*6, 5, 100-A, ST7735_WHITE);  
    tft.drawFastVLine(20+(ii-10)*6, 5, 100-A, ST7735_WHITE);
    //for(int idx=9;idx>=0;idx--){ 
    //  tft.drawFastHLine(18, 15+10*(8-idx), 141, 0b1110111101011101);
    //}
    tft.drawFastVLine(18+(ii-10)*6, 105-A, A, ST7735_RED);
    tft.drawFastVLine(19+(ii-10)*6, 105-A, A, ST7735_RED);  
    tft.drawFastVLine(20+(ii-10)*6, 105-A, A, ST7735_RED);
    
  }
}


long read_ADC5(){
  bitClear(PRR, PRADC);
  ADCSRA |= bit(ADEN); // Enable the ADC
  long result;
  // Read 1.1V reference against Vcc
  #if defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0); // For ATtiny84
  #else
    ADMUX = _BV(REFS0) | _BV(MUX2) | _BV(MUX0); // For ATmega328 - int. Ref AVcc - Ch.5
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

