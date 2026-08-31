//driver for NCM109+NCT4XX series (register HV5122)
//driver version 1.2
//v 1.2 04.08.2018
//fixed: dots mixed up
//v 1.1 29/06/2018 
//fixed: blink bug fixed
//1 on register's output will turn on a digit 

#include "doIndication101.h"

#define UpperDotsMask 0x2000000
#define LowerDotsMask 0x1000000

void TurnOffAllTubes()
{
  digitalWrite(NIXIE_GROUP_1_PIN, LOW); 
  digitalWrite(NIXIE_GROUP_2_PIN, LOW); 
  digitalWrite(NIXIE_GROUP_3_PIN, LOW);
}

void doIndication()
{
  static byte AnodesGroup=1;
  unsigned long AnodesGroupMask;
  static unsigned long lastTimeInterval1Started = 0;
  if ((micros()-lastTimeInterval1Started)>5000)
  {
   lastTimeInterval1Started=micros();
   if (NightMode) {TurnOffAllTubes(); return;}

   int curTube=AnodesGroup*2-2;

    //uint8_t firstDigit=stringToDisplay.substring(curTube, curTube+1).toInt();
    uint8_t firstDigit  = stringToDisplay.charAt(curTube) - '0';
    uint8_t secondDigit = stringToDisplay.charAt(curTube + 1) - '0';

    ID1_data = ((secondDigit & 0x0F) << 4) | (firstDigit & 0x0F);

    ID1_data |= doEditBlink(curTube); 
    ID1_data |= doEditBlink(curTube + 1) << 4; 
  
    digitalWrite(NIXIE_GROUP_1_PIN, LOW); digitalWrite(NIXIE_GROUP_2_PIN, LOW); digitalWrite(NIXIE_GROUP_3_PIN, LOW);

    digitalWrite(LEpin, LOW);    // allow data input (Transparent mode)

    SPI.transfer(ID1_data);

    digitalWrite(LEpin, HIGH);     // latching data 

    switch (AnodesGroup)
    {
      case 1:{digitalWrite(NIXIE_GROUP_1_PIN, HIGH); digitalWrite(NIXIE_GROUP_2_PIN, LOW); digitalWrite(NIXIE_GROUP_3_PIN, LOW); break;};
      case 2:{digitalWrite(NIXIE_GROUP_1_PIN, LOW); digitalWrite(NIXIE_GROUP_2_PIN, HIGH); digitalWrite(NIXIE_GROUP_3_PIN, LOW); break;};
      case 3:{digitalWrite(NIXIE_GROUP_1_PIN, LOW); digitalWrite(NIXIE_GROUP_2_PIN, LOW); digitalWrite(NIXIE_GROUP_3_PIN, HIGH); break;};
    }

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

    if (blinkState) tmp = 0;
      else tmp = blinkMask;
  }
  if (((dotPattern&~tmp)>>6)&1==1) /*LD=true;*/ digitalWrite(LOWER_DOTS_PIN, HIGH);
      else /*LD=false;*/ digitalWrite(LOWER_DOTS_PIN, LOW);
  if (((dotPattern&~tmp)>>7)&1==1) /*UD=true;*/ digitalWrite(UPPER_DOTS_PIN, HIGH);
      else /*UD=false;*/ digitalWrite(UPPER_DOTS_PIN, LOW);
      
  if ((blinkState==true) && (lowBit==1)) mask=TubeOff;//mask=B11111111;
  return mask;
}

void SPI_Init()
{
  SPI.begin();
  //SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
  SPI.beginTransaction(SPISettings(250000, MSBFIRST, SPI_MODE0));

  pinMode(NIXIE_GROUP_1_PIN, OUTPUT);
  pinMode(NIXIE_GROUP_2_PIN, OUTPUT);
  pinMode(NIXIE_GROUP_3_PIN, OUTPUT);
}
