//NIXIE COUNTDOWN TIMER FOR NCT818 by GRA & AFCH (fominalec@gmail.com)

//v1.0 23.04.2021
//***********************************
//Mode button - Start/Pause
//Mode(LONG) - Set
//Up button - Increase
//Down button - Decrease

#include <SPI.h>
#include <Tone.h>
#include <ClickButton.h>
const byte BlueLedPin = 3; 
const byte pinBuzzer = 2;
const byte pinSet = A0;
const byte pinUp = A2;
const byte pinDown = A1;
const byte LEpin=10; //pin Latch Enabled data accepted while HI level

#define RHV5222PIN 8
bool HV5222;
uint8_t blinkMask=0;

String stringToDisplay="00000000";// Conten of this string will be displayed on tubes (must be 8 chars length)

//                            0      1      2      3      4      5      6      7      8       9
unsigned int SymbolArray[10]={1,     2,     4,     8,     16,    32,    64,    128,   256,    512};

Tone tone1;

//buttons pins declarations
ClickButton setButton(pinSet, LOW, CLICKBTN_PULLUP);
ClickButton upButton(pinUp, LOW, CLICKBTN_PULLUP);
ClickButton downButton(pinDown, LOW, CLICKBTN_PULLUP);
///////////////////

/*******************************************************************************************************
Init Programm
*******************************************************************************************************/
void setup() 
{
  Serial.begin(115200);
  Serial.println(F("Сountdown timer for NCT818_1K ver. 1.0"));
  
  pinMode(LEpin, OUTPUT);
  pinMode(pinSet,  INPUT_PULLUP);
  pinMode(pinUp,  INPUT_PULLUP);
  pinMode(pinDown,  INPUT_PULLUP);
  pinMode(pinBuzzer, OUTPUT);

  // SPI setup
  SPI_Init();
  //stringToDisplay="006000";
  doIndication();
  //pinMode(BlueLedPin, OUTPUT);
  analogWrite(BlueLedPin, 50);
  tone1.begin(pinBuzzer);
  //tone1.play(1000, 100);

  setCounter();

  //buttons objects inits
  setButton.debounceTime   = 20;   // Debounce timer in ms
  setButton.multiclickTime = 30;  // Time limit for multi clicks
  setButton.longClickTime  = 1000; // time until "held-down clicks" register

  upButton.debounceTime   = 20;   // Debounce timer in ms
  upButton.multiclickTime = 30;  // Time limit for multi clicks
  upButton.longClickTime  = 2000; // time until "held-down clicks" register

  downButton.debounceTime   = 20;   // Debounce timer in ms
  downButton.multiclickTime = 30;  // Time limit for multi clicks
  downButton.longClickTime  = 2000; // time until "held-down clicks" register
}

/***************************************************************************************************************
MAIN Programm
***************************************************************************************************************/
int32_t counter=0;
bool isRunning=false;
bool editMode=false;
int editPosition=0;
unsigned long prevTickTime=millis();
int minutes, seconds;
bool LD,UD;
uint8_t digitsArray[8]={0,0,0,0,5,9,0,0};
//uint8_t digitsArray[8]={1,2,3,4,5,6,4,8};
//                      h h:h h:m m:s s
int limit=9;
unsigned long timePassed=0;

void loop()
{

 if (editMode == true)
 {
  blinkMask=1<<editPosition;
 }
 else blinkMask=0;
 
 upButton.Update();
 setButton.Update();
 downButton.Update();
 if ((setButton.clicks < 0)) 
 {
    editMode=true;
    isRunning=false;
    setArray();
    tone1.play(1000, 100);
    Serial.println("Edit Mode ON");
 }
 if (editPosition>7) 
 {
  editPosition=0; 
  editMode=false;
  setCounter();
  Serial.println("Edit Mode OFF");
 }

 if ((setButton.clicks > 0)) 
 {
  tone1.play(1000, 100);
  if (editMode == true)
  {
    editPosition++;
    Serial.print("EditPosition = ");
    Serial.println(editPosition);
  } else
  {
    isRunning=!isRunning;
    Serial.print("Timer is: ");
    Serial.println(isRunning);
  }
 }

 if (editMode == true)
 {
  if (upButton.clicks > 0)
  {
    tone1.play(1000, 100);
    if ((editPosition == 4) || (editPosition == 6)) limit=5; //tens of minutes or seconds
      else limit=9;
    if (digitsArray[editPosition] < limit) digitsArray[editPosition]++;
      else digitsArray[editPosition] = 0;
    printDigitsArray();
    updateTubes();
  }

  if (downButton.clicks > 0)
  {
    tone1.play(1000, 100);
    if ((editPosition == 4) || (editPosition == 6)) limit=5; //tens of minutes or seconds
      else limit=9;
    if (digitsArray[editPosition] > 0) digitsArray[editPosition]--;
      else digitsArray[editPosition] = limit;
    printDigitsArray();
    updateTubes();
  }
 }

 if (isRunning) doDotBlink();
 timePassed=millis()-prevTickTime;
 if ((timePassed>1000) && (isRunning))
 {
  prevTickTime=millis();
  //Serial.println("tick");
  //Serial.println(counter);
  counter=counter-1;
  if (counter<=0) 
  {
    counter=0;
    isRunning=false;
    Serial.println(F("Stop"));
    tone1.play(400, 1000);
  }
  updateTubes();
  //printDigitsArray();
 }
 updateTubes();
}

void setCounter()
{
  counter = (digitsArray[0]*1000UL + digitsArray[1]*100UL + digitsArray[2]*10UL + digitsArray[3])*3600UL + (digitsArray[4]*10UL + digitsArray[5])*60UL + digitsArray[6]*10UL + digitsArray[7];
}
void printDigitsArray()
{
  Serial.println(digitsArray[0]*10000000UL + digitsArray[1]*1000000UL + digitsArray[2]*100000UL + digitsArray[3]*10000UL + digitsArray[4]*1000UL + digitsArray[5]*100UL + digitsArray[6]*10UL + digitsArray[7]);
}

void setArray()
{
  String tmpStr=PreZero4(counter/3600)+PreZero((counter%3600)/60)+PreZero(counter%60);
  uint32_t tmpInt=tmpStr.toInt();
  digitsArray[7]=tmpInt%10;
  tmpInt=tmpInt/10;
  digitsArray[6]=tmpInt%10;
  tmpInt=tmpInt/10;
  digitsArray[5]=tmpInt%10;
  tmpInt=tmpInt/10;
  digitsArray[4]=tmpInt%10;
  tmpInt=tmpInt/10;
  digitsArray[3]=tmpInt%10;
  tmpInt=tmpInt/10;
  digitsArray[2]=tmpInt%10;
  tmpInt=tmpInt/10;
  digitsArray[1]=tmpInt%10;
  tmpInt=tmpInt/10;
  digitsArray[0]=tmpInt;
  Serial.print("digitsArray=");
  printDigitsArray();
}

void updateTubes()
{
  if (editMode == true)
  {
    stringToDisplay=String(digitsArray[0]*10000000UL + digitsArray[1]*1000000UL + digitsArray[2]*100000UL + digitsArray[3]*10000UL + digitsArray[4]*1000UL + digitsArray[5]*100UL + digitsArray[6]*10UL + digitsArray[7]);
  }
    else
    {
      stringToDisplay=PreZero4(counter/3600)+PreZero((counter%3600)/60)+PreZero(counter%60);
    }
  //Serial.print("str=");
  //Serial.println(stringToDisplay);
  doIndication();
}

void doDotBlink()
{
  static unsigned long lastTimeBlink = millis();
  static bool dotState = 0;
  if ((millis() - lastTimeBlink) > 500)
  {
    lastTimeBlink = millis();
    dotState = !dotState;
    if (dotState)
    {
      //dotPattern = B11000000;
      LD=true;
      UD=true;
    }
    else
    {
      //dotPattern = B00000000;
      LD=false;
      UD=false;
    }
    //updateTubes();
  }
}

String PreZero(int digit)
{
  //digit = abs(digit);
  if (digit < 10) return String("0") + String(digit);
  else return String(digit);
}

String PreZero4(uint16_t digit)
{
  //digit = abs(digit);
  if (digit < 10) return String("000") + String(digit);
  if (digit < 100) return String("00") + String(digit);
  if (digit < 1000) return String("0") + String(digit);
  else return String(digit);
}

#define UpperDotsMask 0x80000000
#define LowerDotsMask 0x40000000
#define TubeON 0xFFFF
#define TubeOFF 0x3C00

void doIndication()
{
  
  static unsigned long lastTimeInterval1Started;
  if ((micros()-lastTimeInterval1Started)<2000) return;
  lastTimeInterval1Started=micros();
    
  unsigned long Var32=0;
  
  long digits=stringToDisplay.toInt();
  
  /**********************************************************
   * Подготавливаем данные по 32бита 3 раза
   * Формат данных [H1][H2}[M1][M2][S1][Y1][Y2]
   *********************************************************/
  //-------- REG 2 ----------------------------------------------- 
  Var32|=(unsigned long)(SymbolArray[digits%10]&doEditBlink(7)&moveMask())<<10; // Y2
  digits=digits/10;

  Var32 |= (unsigned long)SymbolArray[digits%10]&doEditBlink(6)&moveMask();//y1
  digits=digits/10;

  if (LD) Var32&=~LowerDotsMask;
    else  Var32|=LowerDotsMask;
  
  if (UD) Var32&=~UpperDotsMask; 
    else  Var32|=UpperDotsMask; 
    

  digitalWrite(LEpin, LOW);    

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

 //-------- REG 1 ----------------------------------------------- 
  Var32=0;
 
  Var32|=(unsigned long)(SymbolArray[digits%10]&doEditBlink(5)&moveMask())<<20; // s2
  digits=digits/10;

  Var32|=(unsigned long)(SymbolArray[digits%10]&doEditBlink(4)&moveMask())<<10; //s1
  digits=digits/10;

  Var32|=(unsigned long) (SymbolArray[digits%10]&doEditBlink(3)&moveMask()); //m2
  digits=digits/10;

  if (LD) Var32&=~LowerDotsMask;
    else  Var32|=LowerDotsMask;
  
  if (UD) Var32&=~UpperDotsMask; 
    else  Var32|=UpperDotsMask; 

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

  if (LD) Var32&=~LowerDotsMask;  
    else  Var32|=LowerDotsMask;
  
  if (UD) Var32&=~UpperDotsMask; 
    else  Var32|=UpperDotsMask; 
     
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
  //if (!BlinkUp) return 0xFFFF;
  //if (!BlinkDown) return 0xFFFF;
  int lowBit=blinkMask>>pos;
  lowBit=lowBit&B00000001;
  
  static unsigned long lastTimeEditBlink=millis();
  static bool blinkState=false;
  word mask=0xFFFF;
  static int tmp=0;//blinkMask;
  if ((millis()-lastTimeEditBlink)>300) 
  {
    lastTimeEditBlink=millis();
    blinkState=!blinkState;
    if (blinkState) tmp= 0;
      else tmp=blinkMask;
  }
      
  if ((blinkState==true) && (lowBit==1)) mask=0x3C00;//mask=B11111111;
  return mask;
}

word moveMask()
{
  #define tubesQuantity 3
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
