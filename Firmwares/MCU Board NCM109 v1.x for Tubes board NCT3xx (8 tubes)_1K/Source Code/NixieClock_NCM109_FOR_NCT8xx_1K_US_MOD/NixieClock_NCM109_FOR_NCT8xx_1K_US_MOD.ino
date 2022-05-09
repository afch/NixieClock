const String FirmwareVersion = "017000";
//Format                _X.XX__
#define HardwareVersion "MCU109 for 3XX on 8 tubes US(1K)" 
//NIXIE CLOCK NCM107, NCM109(for NCT318 v1.1 + NCT818 v1.0) by GRA & AFCH (fominalec@gmail.com)
//1.70 09.05.2022
//Fixed: year value bug
//1.69 18.02.2022
//Fixed: min value bug
//1.68 14.02.2020
//Improvements: freed up some memory
//1.67 22.12.2020 
// The driver has been changed to support BOTH HV5122 and HV 5222 (switching using resistor R5222 Arduino pin No. 8)
//SPI initialization moved to SPI_Init()
//1.66 08.04.2020
//Fixed: Seconds setting bug
//1.65 01.24.2020
//Added: DS3231 internal temperature sensor self test: 5 beeps if fail.
//1.64 15/11/2019
//Added: Millis on 8th tubes
//Fixed: millis while in 12 hours time format
//1.63 1K Ohm version 31/07/2019
//1.63 05/11/2017
//Added: LEDs brightness adjustments.
//1.62 04/11/2017
//Added: Modes changer settings in main menu
//1.61
//Added: temperature adjustemnts    
//1.60
//Added: temperature reading
//1.032 
//Added: time sync with RTC each 60 seconds
//1.03.1 10.05.2017
//Added: Antipoisoning effect
//1.02 17.10.2016
//Fixed: RGB color controls
//Update to Arduino IDE 1.6.12 (Time.h replaced to TimeLib.h)
//1.01
//Added RGB LEDs lock(by UP and Down Buttons)
//Added Down and Up buttons pause and resume self testing
//25.09.2016 update to HW ver 1.1
//25.05.2016

#define tubes8
//#define tubes6
//#define tubes4

#define BOARD_MODEL_MCU107
//#define BOARD_MODEL_NCS312
//#define BOARD_MODEL_NCS314_V2

#include <SPI.h>
#include <Wire.h>
#include <ClickButton.h>
#include <TimeLib.h>
#include <Tone.h>
#include <EEPROM.h>
#include "doIndication818.h"
#include <OneWire.h>
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
#include <IRremote.h>
#endif

boolean UD, LD; // DOTS control;

byte data[12];
byte addr[8];
int celsius, fahrenheit;

#define HIZpin 8 //pin Z state in registers outputs (while LOW level)
#define DHVpin 5 // off/on MAX1771 Driver  Hight Voltage(DHV) 110-220V
#define RedLedPin 9 //MCU WDM output for red LEDs 9-g
#define GreenLedPin 6 //MCU WDM output for green LEDs 6-b
#define BlueLedPin 3 //MCU WDM output for blue LEDs 3-r
#define pinSet A0
#define pinUp A2
#define pinDown A1
#define pinBuzzer 2
#define pinUpperDots 12 //HIGH value light a dots
#define pinLowerDots 8 //HIGH value light a dots

bool RTC_present;
#ifdef BOARD_MODEL_MCU107
const byte pinTemp = A3;
#endif
#ifdef BOARD_MODEL_NCS312
const byte pinTemp = 7;
#endif
OneWire  ds(pinTemp);
String TempDegrees;
bool TempPresent = false;
#define CELSIUS 0
#define FAHRENHEIT 1
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
int RECV_PIN = 4;
IRrecv irrecv(RECV_PIN);
decode_results results;
#endif

String stringToDisplay = "000000";// Conten of this string will be displayed on tubes (must be 6 chars length)
int menuPosition = 0; // 0 - time
// 1 - date
// 2 - alarm
// 3 - 12/24 hours mode

int blinkMask = B00000000; //bit mask for blinkin digits (1 - blink, 0 - constant light)
int blankMask = B00000000; //bit mask for digits (1 - off, 0 - on)

byte dotPattern = B00000000; //bit mask for separeting dots
//B00000000 - turn off up and down dots
//B1100000 - turn off all dots

#define DS1307_ADDRESS 0x68
byte zero = 0x00; //workaround for issue #527
int RTC_hours, RTC_minutes, RTC_seconds, RTC_day, RTC_month, RTC_year, RTC_day_of_week;

#define TimeIndex        0
#define DateIndex        1
#define AlarmIndex       2
#define hModeIndex       3
#define TemperatureIndex 4
#define ModeChangeIndex  5
#define LEDsIndex        6
#define TimeHoursIndex   7
#define TimeMintuesIndex 8
#define TimeSecondsIndex 9
#define DateMonthIndex   10
#define DateDayIndex     11
#define DateYearIndex    12
#define AlarmHourIndex   13
#define AlarmMinuteIndex 14
#define AlarmSecondIndex 15
#define Alarm01          16
#define hModeValueIndex  17
#define DegreesFormatIndex 18
#define TempAdjustIndex  19
#define ModeChangeInIndex 20
#define ModeChangeOutIndex 21
#define LEDsBrightnessIndex   22

#define FirstParent      TimeIndex
#define LastParent       LEDsIndex
#define SettingsCount    (LEDsBrightnessIndex+1)
#define NoParent         0
#define NoChild          0


//-------------------------------0--------1--------2-------3--------4---------5--------6--------7--------8--------9--------10-------11- ------12-------13-------14-------15---------16-------17-----------18-----------19----------20-------21---------------22
//                     names:  Time,   Date,   Alarm,   12/24, Temperature,ModeChange, LEDs   hours,   mintues, seconds,  day,    month,   year,    hour,   minute,   second alarm01  hour_format DegreesFormat  TempAdj  ModeChangeIn  ModeChangeOut  LEDsBrightness 
//                               1        1        1       1        1         1        1        1        1        1        1        1        1        1        1        1        1          1             1           1         1           1
byte parent[SettingsCount] = {NoParent, NoParent, NoParent, NoParent, NoParent,NoParent,NoParent,1,       1,       1,       2,       2,       2,       3,       3,       3,       3,         4,            5,          5,        6,          6,              7};
byte firstChild[SettingsCount] = {7,      10,       13,     17,      18,       20,      22,      0,       0,       0,       0,       0,       0,       0,       0,       0,       0,         0,            0,          0,      NoChild,      NoChild,        NoChild};
byte lastChild[SettingsCount] = { 9,      12,       16,     17,      19,       21,      22,      0,       0,       0,       0,       0,       0,       0,       0,       0,       0,         0,            0,          0,      NoChild,      NoChild,        NoChild};
int value[SettingsCount] = {      0,       0,       0,      0,       0,        0,       0,       0,       0,       0,       0,       0,       0,       0,       0,       0,       0,        24,            0,          0,        1,          5,              99};
int maxValue[SettingsCount] = {   0,       0,       0,      0,       0,        0,       0,       23,      59,      59,      12,      31,      99,      23,      59,      59,      1,        24,       FAHRENHEIT,      99,       99,         99,             99};
int minValue[SettingsCount] = {   0,       0,       0,      12,      0,        0,       0,       00,      00,      00,       1,       1,      00,      00,      00,      00,      0,        12,         CELSIUS,      -99,       0,          0,              0};
byte blinkPattern[SettingsCount] = {
  B00000000, //0
  B00000000, //1
  B00000000, //2
  B00000000, //3
  B00000000, //4
  B00000000, //5
  B00000000, //6
  B00000011, //7
  B00001100, //8
  B00110000, //9
  B00000011, //10
  B00001100, //11
  B00110000, //12
  B00000011, //13
  B00001100, //14
  B00110000, //15
  B11000000, //16
  B00001100, //17
  B00011110, //18
  B00000011, //19
  B00000011, //20
  B00001100, //21
  B00001100  //22
};


bool editMode = false;

long downTime = 0;
long upTime = 0;
const long settingDelay = 150;
bool BlinkUp = false;
bool BlinkDown = false;
unsigned long enteringEditModeTime = 0;
bool RGBLedsOn = true;
byte RGBLEDsEEPROMAddress = 0;
byte HourFormatEEPROMAddress = 1;
byte AlarmTimeEEPROMAddress = 2; //3,4,5
byte AlarmArmedEEPROMAddress = 6;
byte LEDsLockEEPROMAddress = 7;
byte LEDsRedValueEEPROMAddress = 8;
byte LEDsGreenValueEEPROMAddress = 9;
byte LEDsBlueValueEEPROMAddress = 10;
byte DegreesFormatEEPROMAddress = 11;
byte TempAdjustEEPROMAddress = 12; //and 13
byte ModeChangeInEEPROMAddress = 14;
byte ModeChangeOutEEPROMAddress = 15;
byte LEDsBrightnessEEPROMAddress = 16;

//buttons pins declarations
ClickButton setButton(pinSet, LOW, CLICKBTN_PULLUP);
ClickButton upButton(pinUp, LOW, CLICKBTN_PULLUP);
ClickButton downButton(pinDown, LOW, CLICKBTN_PULLUP);
///////////////////

Tone tone1;
#define isdigit(n) (n >= '0' && n <= '9')
//char *song = "MissionImp:d=16,o=6,b=95:32d,32d#,32d,32d#,32d,32d#,32d,32d#,32d,32d,32d#,32e,32f,32f#,32g,g,8p,g,8p,a#,p,c7,p,g,8p,g,8p,f,p,f#,p,g,8p,g,8p,a#,p,c7,p,g,8p,g,8p,f,p,f#,p,a#,g,2d,32p,a#,g,2c#,32p,a#,g,2c,a#5,8c,2p,32p,a#5,g5,2f#,32p,a#5,g5,2f,32p,a#5,g5,2e,d#,8d";
char *song = "PinkPanther:d=4,o=5,b=160:8d#,8e,2p,8f#,8g,2p,8d#,8e,16p,8f#,8g,16p,8c6,8b,16p,8d#,8e,16p,8b,2a#,2p,16a,16g,16e,16d,2e";
//char *song="VanessaMae:d=4,o=6,b=70:32c7,32b,16c7,32g,32p,32g,32p,32d#,32p,32d#,32p,32c,32p,32c,32p,32c7,32b,16c7,32g#,32p,32g#,32p,32f,32p,16f,32c,32p,32c,32p,32c7,32b,16c7,32g,32p,32g,32p,32d#,32p,32d#,32p,32c,32p,32c,32p,32g,32f,32d#,32d,32c,32d,32d#,32c,32d#,32f,16g,8p,16d7,32c7,32d7,32a#,32d7,32a,32d7,32g,32d7,32d7,32p,32d7,32p,32d7,32p,16d7,32c7,32d7,32a#,32d7,32a,32d7,32g,32d7,32d7,32p,32d7,32p,32d7,32p,32g,32f,32d#,32d,32c,32d,32d#,32c,32d#,32f,16c";
//char *song="DasBoot:d=4,o=5,b=100:d#.4,8d4,8c4,8d4,8d#4,8g4,a#.4,8a4,8g4,8a4,8a#4,8d,2f.,p,f.4,8e4,8d4,8e4,8f4,8a4,c.,8b4,8a4,8b4,8c,8e,2g.,2p";
//char *song="Scatman:d=4,o=5,b=200:8b,16b,32p,8b,16b,32p,8b,2d6,16p,16c#.6,16p.,8d6,16p,16c#6,8b,16p,8f#,2p.,16c#6,8p,16d.6,16p.,16c#6,16b,8p,8f#,2p,32p,2d6,16p,16c#6,8p,16d.6,16p.,16c#6,16a.,16p.,8e,2p.,16c#6,8p,16d.6,16p.,16c#6,16b,8p,8b,16b,32p,8b,16b,32p,8b,2d6,16p,16c#.6,16p.,8d6,16p,16c#6,8b,16p,8f#,2p.,16c#6,8p,16d.6,16p.,16c#6,16b,8p,8f#,2p,32p,2d6,16p,16c#6,8p,16d.6,16p.,16c#6,16a.,16p.,8e,2p.,16c#6,8p,16d.6,16p.,16c#6,16a,8p,8e,2p,32p,16f#.6,16p.,16b.,16p.";
//char *song="Popcorn:d=4,o=5,b=160:8c6,8a#,8c6,8g,8d#,8g,c,8c6,8a#,8c6,8g,8d#,8g,c,8c6,8d6,8d#6,16c6,8d#6,16c6,8d#6,8d6,16a#,8d6,16a#,8d6,8c6,8a#,8g,8a#,c6";
//char *song="WeWishYou:d=4,o=5,b=200:d,g,8g,8a,8g,8f#,e,e,e,a,8a,8b,8a,8g,f#,d,d,b,8b,8c6,8b,8a,g,e,d,e,a,f#,2g,d,g,8g,8a,8g,8f#,e,e,e,a,8a,8b,8a,8g,f#,d,d,b,8b,8c6,8b,8a,g,e,d,e,a,f#,1g,d,g,g,g,2f#,f#,g,f#,e,2d,a,b,8a,8a,8g,8g,d6,d,d,e,a,f#,2g";
#define OCTAVE_OFFSET 0
char *p;

int notes[] = { 0,
                NOTE_C4, NOTE_CS4, NOTE_D4, NOTE_DS4, NOTE_E4, NOTE_F4, NOTE_FS4, NOTE_G4, NOTE_GS4, NOTE_A4, NOTE_AS4, NOTE_B4,
                NOTE_C5, NOTE_CS5, NOTE_D5, NOTE_DS5, NOTE_E5, NOTE_F5, NOTE_FS5, NOTE_G5, NOTE_GS5, NOTE_A5, NOTE_AS5, NOTE_B5,
                NOTE_C6, NOTE_CS6, NOTE_D6, NOTE_DS6, NOTE_E6, NOTE_F6, NOTE_FS6, NOTE_G6, NOTE_GS6, NOTE_A6, NOTE_AS6, NOTE_B6,
                NOTE_C7, NOTE_CS7, NOTE_D7, NOTE_DS7, NOTE_E7, NOTE_F7, NOTE_FS7, NOTE_G7, NOTE_GS7, NOTE_A7, NOTE_AS7, NOTE_B7
              };

int fireforks[] = {0, 0, 1, //1
                   -1, 0, 0, //2
                   0, 1, 0, //3
                   0, 0, -1, //4
                   1, 0, 0, //5
                   0, -1, 0
                  }; //array with RGB rules (0 - do nothing, -1 - decrese, +1 - increse

void setRTCDateTime(byte h, byte m, byte s, byte d, byte mon, byte y, byte w = 1);

int functionDownButton = 0;
int functionUpButton = 0;
bool LEDsLock = false;
//antipoisoning transaction
bool modeChangedByUser = false;
bool transactionInProgress = false;
//#define timeModePeriod 60000
//#define dateModePeriod 5000
long modesChangePeriod = value[ModeChangeInIndex] * 60000;
#ifdef tubes8
const byte tubesQty = 8;
#endif
#ifdef tubes6
const byte tubesQty = 6;
#endif
#ifdef tubes4
const byte tubesQty = 4;
#endif
//*antipoisoning transaction
bool LEDsBlink=false;
/*******************************************************************************************************
  Init Programm
*******************************************************************************************************/
void setup()
{
  pinMode(DHVpin, OUTPUT);
  digitalWrite(DHVpin, LOW);    // off MAX1771 Driver  Hight Voltage(DHV) 110-220V
  Wire.begin();
  //setRTCDateTime(23,40,00,25,7,15,1);

  Serial.begin(115200);

#ifdef tubes8
  maxValue[DateYearIndex] = 2099;
  minValue[DateYearIndex] = 2017;
  value[DateYearIndex] = 2017;
  blinkPattern[DateYearIndex] = B11110000;
#endif
  //initials values after first power on
  if (EEPROM.read(HourFormatEEPROMAddress) != 12) value[hModeValueIndex] = 24; else value[hModeValueIndex] = 12;
  if (EEPROM.read(RGBLEDsEEPROMAddress) != 0) RGBLedsOn = true; else RGBLedsOn = false;
  if (EEPROM.read(AlarmTimeEEPROMAddress) == 255) value[AlarmHourIndex] = 0; else value[AlarmHourIndex] = EEPROM.read(AlarmTimeEEPROMAddress);
  if (EEPROM.read(AlarmTimeEEPROMAddress + 1) == 255) value[AlarmMinuteIndex] = 0; else value[AlarmMinuteIndex] = EEPROM.read(AlarmTimeEEPROMAddress + 1);
  if (EEPROM.read(AlarmTimeEEPROMAddress + 2) == 255) value[AlarmSecondIndex] = 0; else value[AlarmSecondIndex] = EEPROM.read(AlarmTimeEEPROMAddress + 2);
  if (EEPROM.read(AlarmArmedEEPROMAddress) == 255) value[Alarm01] = 0; else value[Alarm01] = EEPROM.read(AlarmArmedEEPROMAddress);
  if (EEPROM.read(LEDsLockEEPROMAddress) == 255) LEDsLock = false; else LEDsLock = EEPROM.read(LEDsLockEEPROMAddress);
  if (EEPROM.read(DegreesFormatEEPROMAddress) == 255) value[DegreesFormatIndex] = CELSIUS; else value[DegreesFormatIndex] = EEPROM.read(DegreesFormatEEPROMAddress);
  int tInt;
  EEPROM.get(TempAdjustEEPROMAddress, tInt);
  if (tInt== 255) value[TempAdjustIndex] = 0; else value[TempAdjustIndex] = tInt;
  if (EEPROM.read(ModeChangeInEEPROMAddress) != 255) value[ModeChangeInIndex] = EEPROM.read(ModeChangeInEEPROMAddress);
  if (EEPROM.read(ModeChangeOutEEPROMAddress) != 255) value[ModeChangeOutIndex] = EEPROM.read(ModeChangeOutEEPROMAddress);
  if (EEPROM.read(LEDsBrightnessEEPROMAddress) != 255) value[LEDsBrightnessIndex] = EEPROM.read(LEDsBrightnessEEPROMAddress);

  modesChangePeriod = value[ModeChangeInIndex] * 1000;

  pinMode(RedLedPin, OUTPUT);
  pinMode(GreenLedPin, OUTPUT);
  pinMode(BlueLedPin, OUTPUT);

  tone1.begin(pinBuzzer);  
  song = parseSong(song); 

  pinMode(LEpin, OUTPUT);
  pinMode(HIZpin, OUTPUT);
  digitalWrite(HIZpin, LOW);

  // SPI setup
  SPI_Init();

  #define SS 25;
  //buttons pins inits
  pinMode(pinSet,  INPUT_PULLUP);
  pinMode(pinUp,  INPUT_PULLUP);
  pinMode(pinDown,  INPUT_PULLUP);
  ////////////////////////////
  pinMode(pinBuzzer, OUTPUT);

  //buttons objects inits
  setButton.debounceTime   = 20;   // Debounce timer in ms
  setButton.multiclickTime = 30;  // Time limit for multi clicks
  setButton.longClickTime  = 2000; // time until "held-down clicks" register

  upButton.debounceTime   = 20;   // Debounce timer in ms
  upButton.multiclickTime = 30;  // Time limit for multi clicks
  upButton.longClickTime  = 2000; // time until "held-down clicks" register

  downButton.debounceTime   = 20;   // Debounce timer in ms
  downButton.multiclickTime = 30;  // Time limit for multi clicks
  downButton.longClickTime  = 2000; // time until "held-down clicks" register

  //
  //digitalWrite(DHVpin, HIGH); // on MAX1771 Driver  Hight Voltage(DHV) 110-220V
  // code below must be moved to dotest function
  if ( !ds.search(addr))
  {
    //Serial.println(F("Temp. sensor not found."));
    ds.reset_search();
  } else TempPresent = true;
  if (TempPresent)
  {
    ds.reset();
    ds.select(addr);
    ds.write(0xBE);         // Read Scratchpad

    for (byte i = 0; i < 9; i++) // we need 9 bytes
    {
      data[i] = ds.read();
    }
    int16_t raw = (data[1] << 8) | data[0];
    celsius = (float)raw / 16.0;
    fahrenheit = (float)raw / 16.0 * 1.8 + 32.0;
  } else celsius = 0;
  //to dotest
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  doTest();
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  if (LEDsLock == 1)
  {
    // setLEDsFromEEPROM();
  }
  getRTCTime();
  byte prevSeconds = RTC_seconds;
  unsigned long RTC_ReadingStartTime = millis();
  RTC_present = true;
  while (prevSeconds == RTC_seconds)
  {
    getRTCTime();
    //Serial.println(RTC_seconds);
    if ((millis() - RTC_ReadingStartTime) > 3000)
    {
      //Serial.println(F("Warning! RTC DON'T RESPOND!"));
      RTC_present = false;
      break;
    }
  }
  setTime(RTC_hours, RTC_minutes, RTC_seconds, RTC_day, RTC_month, RTC_year);
  digitalWrite(DHVpin, LOW); // off MAX1771 Driver  Hight Voltage(DHV) 110-220V
  setRTCDateTime(RTC_hours, RTC_minutes, RTC_seconds, RTC_day, RTC_month, RTC_year, 1); //записываем только что считанное время в RTC чтобы запустить новую микросхему
  digitalWrite(DHVpin, HIGH); // on MAX1771 Driver  Hight Voltage(DHV) 110-220V
  
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  irrecv.blink13(false);
  irrecv.enableIRIn(); // Start the receiver
#endif
}

int rotator = 0; //index in array with RGB "rules" (increse by one on each 255 cycles)
int cycle = 0; //cycles counter
int RedLight = 255;
int GreenLight = 0;
int BlueLight = 0;
unsigned long prevTime = 0; // time of lase tube was lit
unsigned long prevTime4FireWorks = 0; //time of last RGB changed
//int minuteL=0; //младшая цифра минут

String updateTemperatureString(float fDegrees, byte tempAdjust = 0);

/***************************************************************************************************************
  MAIN Programm
***************************************************************************************************************/
void loop() {
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  if (irrecv.decode(&results)) {
    //Serial.println(results.value, HEX);
    irrecv.resume(); // Receive the next value
  }
#endif

  if (((millis() % 60000) == 0) && (RTC_present)) //synchronize with RTC every 10 seconds
  {
    getRTCTime();
    setTime(RTC_hours, RTC_minutes, RTC_seconds, RTC_day, RTC_month, RTC_year);
    //Serial.println(F("sync"));
  }

  p = playmusic(p);

  if ((millis() - prevTime4FireWorks) > 5)
  {
    rotateFireWorks(); //change color (by 1 step)
    prevTime4FireWorks = millis();
  }

  if ((menuPosition == TimeIndex) || (modeChangedByUser == false) ) modesChanger();
  doIndication();

  setButton.Update();
  upButton.Update();
  downButton.Update();
  if (editMode == false)
  {
    blinkMask = B00000000;

  } else if ((millis() - enteringEditModeTime) > 60000)
  {
    editMode = false;
    menuPosition = firstChild[menuPosition];
    blinkMask = blinkPattern[menuPosition];
  }
  if (setButton.clicks > 0) //short click
  {
    modeChangedByUser = true;
    p = 0; //shut off music )))
    tone1.play(1000, 100);
    enteringEditModeTime = millis();
    menuPosition = menuPosition + 1;
    if (menuPosition == LastParent + 1) menuPosition = TimeIndex;
    /*Serial.print(F("menuPosition="));
    Serial.println(menuPosition);
    Serial.print(F("value="));
    Serial.println(value[menuPosition]);*/

    blinkMask = blinkPattern[menuPosition];
    if ((parent[menuPosition - 1] != 0) and (lastChild[parent[menuPosition - 1] - 1] == (menuPosition - 1))) //exit from edit mode
    {
      if ((parent[menuPosition - 1] - 1 == 1) && (!isValidDate()))
      {
        menuPosition = DateMonthIndex;
        return;
      }
      editMode = false;
      menuPosition = parent[menuPosition - 1] - 1;
      /*Serial.print(F("Exit to parent position: "));
      Serial.println(menuPosition);*/
      if (menuPosition == TimeIndex) setTime(value[TimeHoursIndex], value[TimeMintuesIndex], value[TimeSecondsIndex], day(), month(), year());
#ifdef tubes8
      if (menuPosition == DateIndex) setTime(hour(), minute(), second(), value[DateDayIndex], value[DateMonthIndex], value[DateYearIndex]);
#endif
#ifdef tubes6
      if (menuPosition == DateIndex) setTime(hour(), minute(), second(), value[DateDayIndex], value[DateMonthIndex], 2000 + value[DateYearIndex]);
#endif
      if (menuPosition == AlarmIndex) {
        EEPROM.write(AlarmTimeEEPROMAddress, value[AlarmHourIndex]);
        EEPROM.write(AlarmTimeEEPROMAddress + 1, value[AlarmMinuteIndex]);
        EEPROM.write(AlarmTimeEEPROMAddress + 2, value[AlarmSecondIndex]);
        EEPROM.write(AlarmArmedEEPROMAddress, value[Alarm01]);
      };
      if (menuPosition == hModeIndex) EEPROM.write(HourFormatEEPROMAddress, value[hModeValueIndex]);
      if (menuPosition == TemperatureIndex)
      {
        EEPROM.write(DegreesFormatEEPROMAddress, value[DegreesFormatIndex]);
      }
      if (menuPosition == TemperatureIndex)
      {
        EEPROM.put(TempAdjustEEPROMAddress, value[TempAdjustIndex]);
       // Serial.println(F("Put TempAdjust="));
       // Serial.println(value[TempAdjustIndex]);
        exit;
      }
      if (menuPosition == ModeChangeIndex)
      {
        EEPROM.write(ModeChangeInEEPROMAddress, value[ModeChangeInIndex]);
        EEPROM.write(ModeChangeOutEEPROMAddress, value[ModeChangeOutIndex]);
        exit;
      }
      if (menuPosition == LEDsIndex)
      {
        //LEDsBlink=false;
        EEPROM.write(LEDsBrightnessEEPROMAddress, value[LEDsBrightnessIndex]);
        exit;
      }
      digitalWrite(DHVpin, LOW); // off MAX1771 Driver  Hight Voltage(DHV) 110-220V
      setRTCDateTime(hour(), minute(), second(), day(), month(), year() % 1000, 1);
      digitalWrite(DHVpin, HIGH); // on MAX1771 Driver  Hight Voltage(DHV) 110-220V
    } // end exit from edit mode
    if (menuPosition != TempAdjustIndex)
      value[menuPosition] = extractDigits(blinkMask);
  }
  if (setButton.clicks < 0) //long click
  {
    tone1.play(1000, 100);
    if (!editMode)
    {
      enteringEditModeTime = millis();
#ifdef tubes8
      if (menuPosition == TimeIndex) stringToDisplay = PreZero(hour()) + PreZero(minute()) + PreZero(second()) + "00"; //temporary enabled 24 hour format while settings
#endif
#ifdef tubes6
      if (menuPosition == TimeIndex) stringToDisplay = PreZero(hour()) + PreZero(minute()) + PreZero(second()); //temporary enabled 24 hour format while settings
#endif
    }
    menuPosition = firstChild[menuPosition];
    /*Serial.print(F("MenuPosition="));
    Serial.println(menuPosition);*/
    if (menuPosition == AlarmHourIndex) {
      value[Alarm01] = 1; dotPattern = B10000000;
    }
    editMode = !editMode;
    blinkMask = blinkPattern[menuPosition];
    if (menuPosition != DegreesFormatIndex)
      value[menuPosition] = extractDigits(blinkMask);
  }

  if (upButton.clicks != 0) functionUpButton = upButton.clicks;

  if (upButton.clicks > 0)
  {
    modeChangedByUser = true;
    p = 0; //shut off music )))
    tone1.play(1000, 100);
    incrementValue();
    if (!editMode)
    {
      LEDsLock = false;
      EEPROM.write(LEDsLockEEPROMAddress, 0);
    }
  }

  if (functionUpButton == -1 && upButton.depressed == true)
  {
    BlinkUp = false;
    if (editMode == true)
    {
      if ( (millis() - upTime) > settingDelay)
      {
        upTime = millis();// + settingDelay;
        incrementValue();
      }
    }
  } else BlinkUp = true;

  if (downButton.clicks != 0) functionDownButton = downButton.clicks;

  if (downButton.clicks > 0)
  {
    modeChangedByUser = true;
    p = 0; //shut off music )))
    tone1.play(1000, 100);
    dicrementValue();
    if (!editMode)
    {
      LEDsLock = true;
      EEPROM.write(LEDsLockEEPROMAddress, 1);
      EEPROM.write(LEDsRedValueEEPROMAddress, RedLight);
      EEPROM.write(LEDsGreenValueEEPROMAddress, GreenLight);
      EEPROM.write(LEDsBlueValueEEPROMAddress, BlueLight);
    }
  }

  if (functionDownButton == -1 && downButton.depressed == true)
  {
    BlinkDown = false;
    if (editMode == true)
    {
      if ( (millis() - downTime) > settingDelay)
      {
        downTime = millis();// + settingDelay;
        dicrementValue();
      }
    }
  } else BlinkDown = true;

  if (!editMode)
  {
    if (upButton.clicks < 0)
    {
      tone1.play(1000, 100);
      RGBLedsOn = true;
      EEPROM.write(RGBLEDsEEPROMAddress, 1);
      //Serial.println(F("RGB=on"));
      setLEDsFromEEPROM();
    }
    if (downButton.clicks < 0)
    {
      tone1.play(1000, 100);
      RGBLedsOn = false;
      EEPROM.write(RGBLEDsEEPROMAddress, 0);
      //Serial.println(F("RGB=off"));
    }
  }


  static bool updateDateTime = false;
  switch (menuPosition)
  {
    case TimeIndex: //time mode
      LEDsBlink=false;
      if (!transactionInProgress) stringToDisplay = updateDisplayString();
      doDotBlink();
      checkAlarmTime();
      blankMask = B00000000;
      break;
    case DateIndex: //date mode
      if (!transactionInProgress) stringToDisplay = updateDateString();
      dotPattern = B10000000; 
      checkAlarmTime();
      blankMask = B00000000;
      break;
    case AlarmIndex: //alarm mode
#ifdef tubes8
      stringToDisplay = PreZero(value[AlarmHourIndex]) + PreZero(value[AlarmMinuteIndex]) + PreZero(value[AlarmSecondIndex]) + "00";
#endif
#ifdef tubes6
      stringToDisplay = PreZero(value[AlarmHourIndex]) + PreZero(value[AlarmMinuteIndex]) + PreZero(value[AlarmSecondIndex]);
      blankMask = B00000000;
#endif
      if (value[Alarm01] == 1) dotPattern = B10000000; //turn on upper dots
      else
      {
        dotPattern = B00000000; //turn off dots
      }
      checkAlarmTime();
      break;
    case hModeIndex: //12/24 hours mode
#ifdef tubes8
      stringToDisplay = "00" + String(value[hModeValueIndex]) + "0000";
      blankMask = B11001111;
#endif
#ifdef tubes6
      stringToDisplay = "00" + String(value[hModeValueIndex]) + "00";
      blankMask = B00110011;
#endif
      dotPattern = B00000000; //turn off all dots
      checkAlarmTime();
      break;
    case TemperatureIndex: //missed break
    case DegreesFormatIndex:
      if (!transactionInProgress)
      {
        stringToDisplay = updateTemperatureString(getTemperature(value[DegreesFormatIndex]), 0);
        if (value[DegreesFormatIndex] == CELSIUS)
        {
          blankMask = B11000111;
          dotPattern = B10000000;
        }
        else
        {
          blankMask = B10001111;
          dotPattern = B11000000;
        }
      }

      if (getTemperature(value[DegreesFormatIndex]) < 0) dotPattern &= B01111111;
       else dotPattern |= B10000000;
      break;
    case TempAdjustIndex:
    stringToDisplay = updateTemperatureString(getTemperature(value[DegreesFormatIndex]), 1);
      if (value[DegreesFormatIndex] == CELSIUS)
        {
          blankMask = B00000111;
          dotPattern = B10000000;
         // blinkPattern[TempAdjustIndex]=B00110000;
          blinkMask=B00000011;
        }
        else
        {
          blankMask = B10000011;
          dotPattern = B11000000;
          //blinkPattern[TempAdjustIndex]=B00000011;
          blinkMask=B00110000;
        }
        if (getTemperature(value[DegreesFormatIndex]) < 0) dotPattern &= B01111111;
       else dotPattern |= B10000000;
    break;
    case ModeChangeIndex:
    case ModeChangeInIndex:
    case ModeChangeOutIndex:
      blankMask = B00000000;
      #ifdef tubes8
      stringToDisplay=PreZero(value[ModeChangeInIndex])+PreZero(value[ModeChangeOutIndex])+String((millis() % 1000) / 100)+String(((millis()+100) % 1000) / 100)+String(((millis()+200) % 1000) / 100)+String(((millis()+300) % 1000) / 100);
      //stringToDisplay=PreZero(value[ModeChangeInIndex])+PreZero(value[ModeChangeOutIndex])+String(millis() % 1000);
      #endif
      #ifdef tubes6
      //stringToDisplay=PreZero(value[ModeChangeInIndex])+PreZero(value[ModeChangeOutIndex])+"00";
      stringToDisplay=PreZero(value[ModeChangeInIndex])+PreZero(value[ModeChangeOutIndex])+String((millis() % 1000) / 100)+String(((millis()+100) % 1000) / 100);
      #endif
    break;
    case LEDsIndex:
    case LEDsBrightnessIndex:
      doLEDsBlink();
      #ifdef tubes8
        stringToDisplay="00"+PreZero(value[LEDsBrightnessIndex])+"0000";
      #endif
      #ifdef tubes6
        stringToDisplay="00"+PreZero(value[LEDsBrightnessIndex])+"0000";
      #endif
    break;
  }
}

String PreZero(int digit)
{
  digit=abs(digit);
  if (digit < 10) return String("0") + String(digit);
  else return String(digit);
}

void rotateFireWorks()
{
  if ((RGBLedsOn==false) || (value[LEDsBrightnessIndex]==0))
  {
    analogWrite(RedLedPin, 0);
    analogWrite(GreenLedPin, 0);
    analogWrite(BlueLedPin, 0);
    return;
  }
  
  if (LEDsBlink==false)
  {
    analogWrite(RedLedPin,   RedLight   * (value[LEDsBrightnessIndex]+1)/100);
    analogWrite(GreenLedPin, GreenLight * (value[LEDsBrightnessIndex]+1)/100);
    analogWrite(BlueLedPin,  BlueLight  * (value[LEDsBrightnessIndex]+1)/100);
  } else 
  {
    analogWrite(RedLedPin, 0);
    analogWrite(GreenLedPin, 0);
    analogWrite(BlueLedPin, 0);
  }

  if (LEDsLock) return;
  RedLight = RedLight + fireforks[rotator * 3];
  GreenLight = GreenLight + fireforks[rotator * 3 + 1];
  BlueLight = BlueLight + fireforks[rotator * 3 + 2];
  cycle = cycle + 1;
  if (cycle == 255)
  {
    rotator = rotator + 1;
    cycle = 0;
  }
  if (rotator > 5) rotator = 0;
}

String updateDisplayString()
{
  static  unsigned long lastTimeStringWasUpdated;
  if ((millis() - lastTimeStringWasUpdated) > 50)
  {
    lastTimeStringWasUpdated = millis();
    return getTimeNow();
  }
  return stringToDisplay;
}

String getTimeNow()
{
#ifdef tubes8
  byte pseudoMillis[10]={0,2,4,6,8,1,3,5,7,9};
  static int pMIndex=0;
  pMIndex++;
  if (pMIndex>9) pMIndex=0;
  if (value[hModeValueIndex] == 24) return PreZero(hour()) + PreZero(minute()) + PreZero(second()) + (millis() % 1000) / 100 + pseudoMillis[pMIndex];
  else return PreZero(hourFormat12()) + PreZero(minute()) + PreZero(second()) + (millis() % 1000) / 100 + pseudoMillis[pMIndex];
#endif
#ifdef tubes6
  if (value[hModeValueIndex] == 24) return PreZero(hour()) + PreZero(minute()) + PreZero(second());
  else return PreZero(hourFormat12()) + PreZero(minute()) + PreZero(second());
#endif
}

void doTest()
{
  Serial.print(F("Firmware: "));
  Serial.println(FirmwareVersion.substring(1, 2) + "." + FirmwareVersion.substring(2, 5));
  Serial.println(HardwareVersion);
  //Serial.println(freeRam());
  //Serial.println(F("Test"));

  p = song;
  parseSong(p);

  analogWrite(RedLedPin, 255);
  delay(1000);
  analogWrite(RedLedPin, 0);
  analogWrite(GreenLedPin, 255);
  delay(1000);
  analogWrite(GreenLedPin, 0);
  analogWrite(BlueLedPin, 255);
  delay(1000);
  //while(1);
#ifdef tubes8
  String testStringArray[11] = {"00000000", "11111111", "22222222", "33333333", "44444444", "55555555", "66666666", "77777777", "88888888", "99999999", ""};
  testStringArray[10] = FirmwareVersion + "00";
#endif
#ifdef tubes6
  String testStringArray[11] = {"000000", "111111", "222222", "333333", "444444", "555555", "666666", "777777", "888888", "999999", ""};
  testStringArray[10] = FirmwareVersion;
#endif
  //testStringArray[12]="00"+PreZero(celsius)+"00";
  //Serial.print(F("Temp = "));
  //Serial.println(celsius);

  int dlay = 500;
  bool test = 1;
  byte strIndex = -1;
  unsigned long startOfTest = millis() + 1000; //disable delaying in first iteration
  //digitalWrite(DHVpin, HIGH);
  bool digitsLock = false;
  while (test)
  {
    if (digitalRead(pinDown)==0) digitsLock=true;
    if (digitalRead(pinUp)==0) digitsLock=false;

    if ((millis()-startOfTest)>dlay) 
     {
       startOfTest=millis();
       if (!digitsLock) strIndex=strIndex+1;
       if (strIndex==10) dlay=3000;
       if (strIndex==11) break;
       stringToDisplay=testStringArray[strIndex];
       //Serial.println(stringToDisplay);
      }
    doIndication();  
   }

   testDS3231TempSensor();
  
}

void doDotBlink()
{
  static unsigned long lastTimeBlink = millis();
  static bool dotState = 0;
  if ((millis() - lastTimeBlink) > 1000)
  {
    lastTimeBlink = millis();
    dotState = !dotState;
    if (dotState)
    {
      dotPattern = B11000000;
    }
    else
    {
      dotPattern = B00000000;
      //Serial.println(updateTemperatureString(CELSIUS));
    }
  }
}

void setRTCDateTime(byte h, byte m, byte s, byte d, byte mon, byte y, byte w)
{
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(zero); //stop Oscillator

  Wire.write(decToBcd(s));
  Wire.write(decToBcd(m));
  Wire.write(decToBcd(h));
  Wire.write(decToBcd(w));
  Wire.write(decToBcd(d));
  Wire.write(decToBcd(mon));
  Wire.write(decToBcd(y));

  Wire.write(zero); //start

  Wire.endTransmission();

}

byte decToBcd(byte val) {
  // Convert normal decimal numbers to binary coded decimal
  return ( (val / 10 * 16) + (val % 10) );
}

byte bcdToDec(byte val)  {
  // Convert binary coded decimal to normal decimal numbers
  return ( (val / 16 * 10) + (val % 16) );
}

void getRTCTime()
{
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(zero);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 7);

  RTC_seconds = bcdToDec(Wire.read());
  RTC_minutes = bcdToDec(Wire.read());
  RTC_hours = bcdToDec(Wire.read() & 0b111111); //24 hour time
  RTC_day_of_week = bcdToDec(Wire.read()); //0-6 -> sunday - Saturday
  RTC_day = bcdToDec(Wire.read());
  RTC_month = bcdToDec(Wire.read());
  RTC_year = bcdToDec(Wire.read());
}


int extractDigits(int b)
{
  String tmp = "1";

  if (b == B00000011)
  {
    tmp = stringToDisplay.substring(0, 2);
  }
  if (b == B00001100)
  {
    tmp = stringToDisplay.substring(2, 4);
  }
  if (b == B00110000)
  {
    tmp = stringToDisplay.substring(4, 6);
  }
  if (b == B11110000)
  {
    tmp = stringToDisplay.substring(4);
  }
  if (b == B11000000)
  {
    tmp = stringToDisplay.substring(6);
  }

  return tmp.toInt();
}

void injectDigits(byte b, int value)
{
  value=abs(value);
  //Serial.println(value);
    if (b == B00000011) stringToDisplay = PreZero(value) + stringToDisplay.substring(2);
    if (b == B00001100) stringToDisplay = stringToDisplay.substring(0, 2) + PreZero(value) + stringToDisplay.substring(4);
    if (b == B00110000) stringToDisplay = stringToDisplay.substring(0, 4) + PreZero(value) + stringToDisplay.substring(6);
    if (b == B11110000) stringToDisplay = stringToDisplay.substring(0, 4) + PreZero(value);
    if (b == B11000000) stringToDisplay = stringToDisplay.substring(0, 6) + PreZero(value);
  //Serial.println(stringToDisplay);
}

bool isValidDate()
{
  int days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (value[DateYearIndex] % 4 == 0) days[1] = 29;
  if (value[DateDayIndex] > days[value[DateMonthIndex] - 1]) return false;
  else return true;

}

byte default_dur = 4;
byte default_oct = 6;
int bpm = 63;
int num;
long wholenote;
long duration;
byte note;
byte scale;
char* parseSong(char *p)
{
  // Absolutely no error checking in here
  // format: d=N,o=N,b=NNN:
  // find the start (skip name, etc)

  while (*p != ':') p++;   // ignore name
  p++;                     // skip ':'

  // get default duration
  if (*p == 'd')
  {
    p++; p++;              // skip "d="
    num = 0;
    while (isdigit(*p))
    {
      num = (num * 10) + (*p++ - '0');
    }
    if (num > 0) default_dur = num;
    p++;                   // skip comma
  }

  // get default octave
  if (*p == 'o')
  {
    p++; p++;              // skip "o="
    num = *p++ - '0';
    if (num >= 3 && num <= 7) default_oct = num;
    p++;                   // skip comma
  }

  // get BPM
  if (*p == 'b')
  {
    p++; p++;              // skip "b="
    num = 0;
    while (isdigit(*p))
    {
      num = (num * 10) + (*p++ - '0');
    }
    bpm = num;
    p++;                   // skip colon
  }

  // BPM usually expresses the number of quarter notes per minute
  wholenote = (60 * 1000L / bpm) * 4;  // this is the time for whole note (in milliseconds)
  return p;
}

// now begin note loop
static unsigned long lastTimeNotePlaying = 0;
char* playmusic(char *p)
{
  if (*p == 0)
  {
    return p;
  }
  if (millis() - lastTimeNotePlaying > duration)
    lastTimeNotePlaying = millis();
  else return p;
  // first, get note duration, if available
  num = 0;
  while (isdigit(*p))
  {
    num = (num * 10) + (*p++ - '0');
  }

  if (num) duration = wholenote / num;
  else duration = wholenote / default_dur;  // we will need to check if we are a dotted note after

  // now get the note
  note = 0;

  switch (*p)
  {
    case 'c':
      note = 1;
      break;
    case 'd':
      note = 3;
      break;
    case 'e':
      note = 5;
      break;
    case 'f':
      note = 6;
      break;
    case 'g':
      note = 8;
      break;
    case 'a':
      note = 10;
      break;
    case 'b':
      note = 12;
      break;
    case 'p':
    default:
      note = 0;
  }
  p++;

  // now, get optional '#' sharp
  if (*p == '#')
  {
    note++;
    p++;
  }

  // now, get optional '.' dotted note
  if (*p == '.')
  {
    duration += duration / 2;
    p++;
  }

  // now, get scale
  if (isdigit(*p))
  {
    scale = *p - '0';
    p++;
  }
  else
  {
    scale = default_oct;
  }

  scale += OCTAVE_OFFSET;

  if (*p == ',')
    p++;       // skip comma for next note (or we may be at the end)

  // now play the note

  if (note)
  {
    tone1.play(notes[(scale - 4) * 12 + note], duration);
    if (millis() - lastTimeNotePlaying > duration)
      lastTimeNotePlaying = millis();
    else return p;
    tone1.stop();
  }
  else
  {
    return p;
  }
  Serial.println(F("Incorrect Song Format!"));
  return 0; //error
}


void incrementValue()
{
  enteringEditModeTime = millis();
  if (editMode == true)
  {
    if (menuPosition != hModeValueIndex) // 12/24 hour mode menu position
      value[menuPosition] = value[menuPosition] + 1; else value[menuPosition] = value[menuPosition] + 12;
    if (value[menuPosition] > maxValue[menuPosition])  value[menuPosition] = minValue[menuPosition];
    if (menuPosition == Alarm01)
    {
      if (value[menuPosition] == 1) /*digitalWrite(pinUpperDots, HIGH);*/dotPattern = B10000000; //turn on all dots
      else /*digitalWrite(pinUpperDots, LOW); */ dotPattern = B00000000; //turn off all dots
    }
    injectDigits(blinkMask, value[menuPosition]);
    /*Serial.print("upValue=");
    Serial.println(value[menuPosition]);
    Serial.print("dotPattern=");
    Serial.println(dotPattern, BIN);*/
  }
}

void dicrementValue()
{
  enteringEditModeTime = millis();
  if (editMode == true)
  {
    if (menuPosition != hModeValueIndex) value[menuPosition] = value[menuPosition] - 1; else value[menuPosition] = value[menuPosition] - 12;
    if (value[menuPosition] < minValue[menuPosition]) value[menuPosition] = maxValue[menuPosition];
    if (menuPosition == Alarm01)
    {
      if (value[menuPosition] == 1) /*digitalWrite(pinUpperDots, HIGH);*/ dotPattern = B10000000; //turn on upper dots включаем верхние точки
      else /*digitalWrite(pinUpperDots, LOW);*/ dotPattern = B00000000; //turn off upper dots
    }
    injectDigits(blinkMask, value[menuPosition]);
    /*Serial.print("downValue=");
    Serial.println(value[menuPosition]);
    Serial.print("dotPattern=");
    Serial.println(dotPattern, BIN);*/
  }
}

bool Alarm1SecondBlock = false;
unsigned long lastTimeAlarmTriggired = 0;
void checkAlarmTime()
{
  if (value[Alarm01] == 0) return;
  if ((Alarm1SecondBlock == true) && ((millis() - lastTimeAlarmTriggired) > 1000)) Alarm1SecondBlock = false;
  if (Alarm1SecondBlock == true) return;
  if ((hour() == value[AlarmHourIndex]) && (minute() == value[AlarmMinuteIndex]) && (second() == value[AlarmSecondIndex]))
  {
    lastTimeAlarmTriggired = millis();
    Alarm1SecondBlock = true;
    Serial.println(F("Wake up, Neo!"));
    p = song;
  }
}

/*int freeRam ()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}*/

void setLEDsFromEEPROM()
{
  digitalWrite(RedLedPin, EEPROM.read(LEDsRedValueEEPROMAddress));
  digitalWrite(GreenLedPin, EEPROM.read(LEDsGreenValueEEPROMAddress));
  digitalWrite(BlueLedPin, EEPROM.read(LEDsBlueValueEEPROMAddress));
}

void modesChanger()
{
  if (editMode == true) return;
  if (value[ModeChangeInIndex] == 0) return;
  static unsigned long lastTimeModeChanged = millis();
  static unsigned long lastTimeAntiPoisoningIterate = millis();
  static int transnumber = 2;
  if ((millis() - lastTimeModeChanged) > modesChangePeriod)
  {
    lastTimeModeChanged = millis();
    if (transnumber == 0) {
      menuPosition = DateIndex;
      modesChangePeriod = value[ModeChangeOutIndex] * 1000;
    }
    if (transnumber == 1) {
      menuPosition = TemperatureIndex;
      modesChangePeriod = value[ModeChangeOutIndex] * 1000;
    }
    if (transnumber == 2) {
      menuPosition = TimeIndex;
      modesChangePeriod = value[ModeChangeInIndex] * 60000;
    }
    transnumber++;
    if (transnumber > 2) transnumber = 0;
    if (modeChangedByUser == true)
    {
      menuPosition = TimeIndex;
    }
    modeChangedByUser = false;
  }
  if ((millis() - lastTimeModeChanged) < 2000)
  {
    if ((millis() - lastTimeAntiPoisoningIterate) > 100)
    {
      lastTimeAntiPoisoningIterate = millis();
      if (menuPosition == TimeIndex) stringToDisplay = antiPoisoning2(updateTemperatureString(getTemperature(value[DegreesFormatIndex])), getTimeNow());
      if (menuPosition == DateIndex) stringToDisplay = antiPoisoning2(getTimeNow(), PreZero(day()) + PreZero(month()) + PreZero(year() % 1000) );
      if (menuPosition == TemperatureIndex) stringToDisplay = antiPoisoning2(PreZero(day()) + PreZero(month()) + PreZero(year() % 1000), updateTemperatureString(getTemperature(value[DegreesFormatIndex])));
      // Serial.println("StrTDInToModeChng="+stringToDisplay);
    }
  } else
  {
    transactionInProgress = false;
  }
}

String antiPoisoning2(String fromStr, String toStr)
{
  //static bool transactionInProgress=false;
  //byte fromDigits[6];
  static byte toDigits[tubesQty];
  static byte currentDigits[tubesQty];
  static byte iterationCounter = 10;
  if (!transactionInProgress)
  {
    transactionInProgress = true;
    blankMask = B00000000;
    for (int i = 0; i < tubesQty; i++)
    {
      currentDigits[i] = fromStr.substring(i, i + 1).toInt();
      toDigits[i] = toStr.substring(i, i + 1).toInt();
    }
  }
  for (int i = 0; i < tubesQty; i++)
  {
    if (iterationCounter < 10) currentDigits[i]++;
    else if (currentDigits[i] != toDigits[i]) currentDigits[i]++;
    if (currentDigits[i] == 10) currentDigits[i] = 0;
  }
  iterationCounter++;
  if (iterationCounter == 20)
  {
    iterationCounter = 0;
    transactionInProgress = false;
  }
  String tmpStr;
  for (int i = 0; i < tubesQty; i++)
    tmpStr += currentDigits[i];
  return tmpStr;
}

String updateDateString()
{
  static unsigned long lastTimeDateUpdate = millis();
  //static String DateString = PreZero(day()) + PreZero(month()) + PreZero(year() % 1000);
  static String DateString = PreZero(month()) + PreZero(day()) + PreZero(year());
  if ((millis() - lastTimeDateUpdate) > 1000)
  {
    lastTimeDateUpdate = millis();
#ifdef tubes8
    //DateString = PreZero(day()) + PreZero(month()) + year();
    DateString = PreZero(month()) + PreZero(day()) + year();
#endif
#ifdef tubes6
    DateString = PreZero(day()) + PreZero(month()) + PreZero(year() % 1000);
#endif
  }
  return DateString;
}

String updateTemperatureString(float fDegrees, byte tempAdjust)
{
  static  unsigned long lastTimeTemperatureString=millis()+1100;
  static String strTemp="00000000";

  if (!TempPresent) return strTemp;
  
  if ((millis() - lastTimeTemperatureString) > 1000)
  {
    lastTimeTemperatureString = millis();
    int iDegrees = round(fDegrees);
    if (value[DegreesFormatIndex] == CELSIUS)
    {
      strTemp = "0" + String(abs(iDegrees)) + "0";
      if (abs(iDegrees) < 1000) strTemp = PreZero(value[TempAdjustIndex]) + String(abs(iDegrees)) + "0";
      if (abs(iDegrees) < 100) strTemp = "0"+ PreZero(value[TempAdjustIndex]) + String(abs(iDegrees)) + "0";
      if (abs(iDegrees) < 10) strTemp = "00" + PreZero(value[TempAdjustIndex]) + String(abs(iDegrees)) + "0";
    }else
    {
      strTemp = "0" + String(abs(iDegrees)) + "0";
      if (abs(iDegrees) < 1000) strTemp = "00" + String(abs(iDegrees)/10) + PreZero(value[TempAdjustIndex]);
      if (abs(iDegrees) < 100) strTemp = "000" + String(abs(iDegrees)/10) + PreZero(value[TempAdjustIndex]);
      if (abs(iDegrees) < 10) strTemp = "0000" + String(abs(iDegrees)/10) + PreZero(value[TempAdjustIndex]);
    }
 
    #ifdef tubes8
      strTemp= ""+strTemp+"00";
    #endif
    return strTemp;
  }
  return strTemp;
}

float getTemperature (boolean bTempFormat)
{
  static float fDegrees;
  static int iterator=0;
  static byte TempRawData[2];

  if (TempPresent==false) return 0;
   
  switch (iterator) 
  {
    case 0: ds.reset(); break;
    case 1: ds.write(0xCC); break; //skip ROM command
    case 2: ds.write(0x44); break; //send make convert to all devices
    case 3: ds.reset(); break;
    case 4: ds.write(0xCC); break; //skip ROM command
    case 5: ds.write(0xBE); break; //send request to all devices
    case 6: TempRawData[0] = ds.read(); break;
    case 7: TempRawData[1] = ds.read(); break;
    default:  break;
  }
  
 if (iterator == 7)
  {
    int16_t raw = (TempRawData[1] << 8) | TempRawData[0];
    if (raw == -1) raw = 0;
    float celsius = (float)raw / 16.0;
    celsius = celsius + (float)value[TempAdjustIndex]/10;//users adjustment
    if (!bTempFormat) fDegrees = celsius * 10;
    else fDegrees = (celsius * 1.8 + 32.0) * 10;
  }
  iterator++;
  if (iterator==8) iterator=0;
  return fDegrees;
}

void doLEDsBlink()
{
  static unsigned long lastTimeBlink = millis();
  if ((millis() - lastTimeBlink) > 333)
  {
    lastTimeBlink = millis();
    LEDsBlink = !LEDsBlink;
  }
}

void testDS3231TempSensor()
{
  int8_t DS3231InternalTemperature=0;
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(0x11);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 2);
  DS3231InternalTemperature=Wire.read();
  //Serial.print(F("DS3231_T="));
  //Serial.println(DS3231InternalTemperature);
  if ((DS3231InternalTemperature<5) || (DS3231InternalTemperature>60)) 
  {
    Serial.println(F("Faulty DS3231!"));
    for (int i=0; i<5; i++)
    {
      tone1.play(1000, 1000);
      delay(2000);
    }
  }
}
