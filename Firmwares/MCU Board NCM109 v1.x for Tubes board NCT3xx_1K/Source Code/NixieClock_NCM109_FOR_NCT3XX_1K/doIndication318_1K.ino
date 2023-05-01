//driver for NCM107+NCT318+NCT818 (registers HV5122)
//ONLY FOR 1K revision!!!!
//1.3 15.05.2020 
//The driver has been changed to support BOTH HV5122 and HV 5222 (switching using resistor R5222 Arduino pin No. 8)
//driver version 1.2 10.09.2018
//Added: new antipoisoning algorithm (work only with 1K revision)
//driver version 1.1 04.08.2018
//Fixed: dots mixed up
//1 on register's output will turn on a digit 

#include "doIndication318_1K.h"

#define UpperDotsMask 0x80000000
#define LowerDotsMask 0x40000000
#define TubeON 0xFFFF
#define TubeOFF 0x3C00

void doIndication()
{
  
  static unsigned long lastTimeInterval1Started;
  if ((micros()-lastTimeInterval1Started)< 3000 /*fpsLimit*/) return;
  //if (menuPosition==TimeIndex) doDotBlink();
  lastTimeInterval1Started=micros();
    
  unsigned long Var32=0;
  
  long digits=stringToDisplay.toInt();
  //long digits=12345678;
  //Serial.print("strtoD=");
  //Serial.println(stringToDisplay);
  
  /**********************************************************
   * Подготавливаем данные по 32бита 3 раза
   * Формат данных [H1][H2}[M1][M2][S1][Y1][Y2]
   *********************************************************/
   
  digitalWrite(LEpin, LOW); 
  //-------- REG 2 ----------------------------------------------- 
  /*Var32|=(unsigned long)(SymbolArray[digits%10]&doEditBlink(7))<<10; // Y2
  digits=digits/10;

  Var32 |= (unsigned long)SymbolArray[digits%10]&doEditBlink(6);//y1
  digits=digits/10;

  if (LD) Var32&=~LowerDotsMask;
    else  Var32|=LowerDotsMask;
  
  if (UD) Var32&=~UpperDotsMask; 
    else  Var32|=UpperDotsMask; 
    
  SPI.transfer(Var32>>24);
  SPI.transfer(Var32>>16);
  SPI.transfer(Var32>>8);
  SPI.transfer(Var32);
 //-------------------------------------------------------------------------
*/
 //-------- REG 1 ----------------------------------------------- 
  Var32=0;
 
  Var32|=(unsigned long)(SymbolArray[digits%10]&doEditBlink(5)&moveMask())<<20; // s2
  digits=digits/10;

  Var32|=(unsigned long)(SymbolArray[digits%10]&doEditBlink(4)&moveMask())<<10; //s1
  digits=digits/10;

  Var32|=(unsigned long) (SymbolArray[digits%10]&doEditBlink(3)&moveMask()); //m2
  digits=digits/10;

  if (LD) Var32|=LowerDotsMask;
    else  Var32&=~LowerDotsMask;
  
  if (UD) Var32|=UpperDotsMask; 
    else Var32&=~UpperDotsMask;

  if (HV5222)
  {
    SPI.transfer(Var32); 
    SPI.transfer(Var32>>8); 
    SPI.transfer(Var32>>16); 
    SPI.transfer(Var32>>24);
  } else
  {
    SPI.transfer(Var32>>24); 
    SPI.transfer(Var32>>16); 
    SPI.transfer(Var32>>8);  
    SPI.transfer(Var32);     
  }
 //-------------------------------------------------------------------------

 //-------- REG 0 ----------------------------------------------- 
  Var32=0;
  
  Var32|=(unsigned long)(SymbolArray[digits%10]&doEditBlink(2)&moveMask())<<20; // m1
  digits=digits/10;

  Var32|= (unsigned long)(SymbolArray[digits%10]&doEditBlink(1)&moveMask())<<10; //h2
  digits=digits/10;

  Var32|= (unsigned long)SymbolArray[digits%10]&doEditBlink(0)&moveMask(); //h1
  digits=digits/10;

  if (LD) Var32|=LowerDotsMask;
    else Var32&=~LowerDotsMask; 
  
  if (UD) Var32|=UpperDotsMask; 
    else Var32&=~UpperDotsMask;
     
  if (HV5222)
  {
    SPI.transfer(Var32); 
    SPI.transfer(Var32>>8); 
    SPI.transfer(Var32>>16); 
    SPI.transfer(Var32>>24);
  } else
  {
    SPI.transfer(Var32>>24); 
    SPI.transfer(Var32>>16); 
    SPI.transfer(Var32>>8);  
    SPI.transfer(Var32);     
  }
  
  digitalWrite(LEpin, HIGH);    
//-------------------------------------------------------------------------
}

word doEditBlink(int pos)
{
  /*
  if (!BlinkUp) return 0;
  if (!BlinkDown) return 0;
  */

  if (!BlinkUp) return TubeON;
  if (!BlinkDown) return TubeON;
  //if (pos==5) return 0xFFFF; //need to be deleted for testing purpose only!
  int lowBit=blinkMask>>pos;
  lowBit=lowBit&B00000001;
  
  static unsigned long lastTimeEditBlink=millis();
  static bool blinkState=false;
  word mask=TubeON;
  static int tmp=0;//blinkMask;
  if ((millis()-lastTimeEditBlink)>300) 
  {
    lastTimeEditBlink=millis();
    blinkState=!blinkState;
    if (blinkState) tmp= 0;
      else tmp=blinkMask;
  }
  if (((dotPattern&~tmp)>>6)&1==1) LD=true;//digitalWrite(pinLowerDots, HIGH);
      else LD=false; //digitalWrite(pinLowerDots, LOW);
  if (((dotPattern&~tmp)>>7)&1==1) UD=true; //digitalWrite(pinUpperDots, HIGH);
      else UD=false; //digitalWrite(pinUpperDots, LOW);
      
  if ((blinkState==true) && (lowBit==1)) mask=TubeOFF;//mask=B11111111;
  //Serial.print("doeditblinkMask=");
  //Serial.println(mask, BIN);
  return mask;
}

word moveMask()
{
  #define tubesQuantity 6
  static int callCounter=0;
  static int tubeCounter=0;
  word onoffTubeMask;
  if (callCounter == tubeCounter) onoffTubeMask=TubeON;
    else onoffTubeMask=TubeOFF;
  callCounter++;
  if (callCounter == tubesQuantity)
  {
    callCounter=0;
    tubeCounter++;
    if (tubeCounter == tubesQuantity) tubeCounter = 0;
  }
  return onoffTubeMask;
}

void SPI_Init()
{
  pinMode(RHV5222PIN, INPUT_PULLUP);
  HV5222=!digitalRead(RHV5222PIN);
  SPI.begin(); //
  if (HV5222)
    SPI.beginTransaction(SPISettings(2000000, LSBFIRST, SPI_MODE2));
    else SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE2));
}
