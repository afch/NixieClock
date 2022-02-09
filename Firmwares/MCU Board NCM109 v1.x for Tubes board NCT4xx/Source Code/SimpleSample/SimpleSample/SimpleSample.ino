 
#include <SPI.h>

const byte LEpin=10; //pin Latch Enabled data accepted while HI level
String stringToDisplay="000000";// Conten of this string will be displayed on tubes (must be 6 chars length)

//                            0      1      2      3      4      5      6      7      8       9
unsigned int SymbolArray[10]={1,     2,     4,     8,     16,    32,    64,    128,   256,    512};
#define AnodeGroup1ON 0x100000
#define AnodeGroup2ON 0x200000
#define AnodeGroup3ON 0x400000


/*******************************************************************************************************
Init Programm
*******************************************************************************************************/
void setup() 
{
  pinMode(LEpin, OUTPUT);

  // SPI setup
  SPI.begin(); //
  SPI.setDataMode (SPI_MODE2);
  SPI.setClockDivider(SPI_CLOCK_DIV8); // SCK = 16MHz/128= 125kHz

  stringToDisplay="123456";
}

/***************************************************************************************************************
MAIN Programm
***************************************************************************************************************/
void loop() {
  
  doIndication(); // To prevent tubes FAILURE call this function as fast as possible!!!
  // !!!!!!!!!! Do not use DELAY nowhere in the program !!!!!!!!

  if (millis()>3000) stringToDisplay="654321"; //change digits 3 seconds after switching on
  
}


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

   var32 |= tmpVar<<10;

   digitalWrite(LEpin, LOW);    // allow data input (Transparent mode)

   SPI.transfer(var32>>24); //[x][x][x][x][x][x][L1][L0]                - L0, L1 - dots
   SPI.transfer(var32>>16); //[x][A2][A1][A0][RC9][RC8][RC7][RC6]       - A0-A2 - anodes
   SPI.transfer(var32>>8);  //[RC5][RC4][RC3][RC2][RC1][RC0][LC9][LC8]  - RC9-RC0 - Right tubes cathodes
   SPI.transfer(var32);     //[LC7][LC6][LC5][LC4][LC3][LC2][LC1][LC0]  - LC9-LC0 - Left tubes cathodes

   digitalWrite(LEpin, HIGH);     // latching data 

   AnodesGroup=AnodesGroup+1;
    if (AnodesGroup==4) {AnodesGroup=1;}
  }
}
