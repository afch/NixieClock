//driver for NCM109+NCT4XX series (register HV5122)
//driver version 1.2
//v 1.2 04.08.2018
//fixed: dots mixed up
//v 1.1 29/06/2018 
//fixed: blink bug fixed
//1 on register's output will turn on a digit 

#include "doIndication109.h"

#define UpperDotsMask 0x2000000
#define LowerDotsMask 0x1000000

void doIndication()
{
  static byte AnodesGroup=1;
  unsigned long AnodesGroupMask;
  static unsigned long lastTimeInterval1Started;
  if ((micros()-lastTimeInterval1Started)>5000)
  {
   lastTimeInterval1Started=micros();
   unsigned long var32=0;
   unsigned long tmpVar; 

  switch (AnodesGroup)
   {
   case 1:{AnodesGroupMask=AnodeGroup1ON; break;};
   case 2:{AnodesGroupMask=AnodeGroup2ON; break;};
   case 3:{AnodesGroupMask=AnodeGroup3ON; break;};
   }

   var32 |= AnodesGroupMask;

   int curTube=AnodesGroup*2-2;
   var32|=SymbolArray[stringToDisplay.substring(curTube, curTube+1).toInt()];
   tmpVar=SymbolArray[stringToDisplay.substring(curTube+1, curTube+2).toInt()];

   var32&=doEditBlink(curTube);  
   tmpVar&=doEditBlink(curTube+1); 

   var32 |= tmpVar<<10;

   if (LD) var32|=LowerDotsMask;
    else var32&=~LowerDotsMask;
  
   if (UD) var32|=UpperDotsMask;
    else var32&=~UpperDotsMask; 

   digitalWrite(LEpin, LOW);    // allow data input (Transparent mode)
   
  if (HV5222)
  {
    SPI.transfer(var32); 
    SPI.transfer(var32>>8); 
    SPI.transfer(var32>>16); 
    SPI.transfer(var32>>24);
  } else
  {
    SPI.transfer(var32>>24); //[x][x][x][x][x][x][L1][L0]                - L0, L1 - dots
    SPI.transfer(var32>>16); //[x][A2][A1][A0][RC9][RC8][RC7][RC6]       - A0-A2 - anodes
    SPI.transfer(var32>>8);  //[RC5][RC4][RC3][RC2][RC1][RC0][LC9][LC8]  - RC9-RC0 - Right tubes cathodes
    SPI.transfer(var32);     //[LC7][LC6][LC5][LC4][LC3][LC2][LC1][LC0]  - LC9-LC0 - Left tubes cathodes
  }

   digitalWrite(LEpin, HIGH);     // latching data 

   AnodesGroup=AnodesGroup+1;
    if (AnodesGroup==4) {AnodesGroup=1;}
  }
}

int doEditBlink(int pos)
{
  if (!BlinkUp) return TubeOn;
  if (!BlinkDown) return TubeOn;
  int lowBit=blinkMask>>pos;
  lowBit=lowBit&B00000001;
  
  static unsigned long lastTimeEditBlink=millis();
  static bool blinkState=false;
  word mask=TubeOn;
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
      
  if ((blinkState==true) && (lowBit==1)) mask=TubeOff;//mask=B11111111;
  return mask;
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
