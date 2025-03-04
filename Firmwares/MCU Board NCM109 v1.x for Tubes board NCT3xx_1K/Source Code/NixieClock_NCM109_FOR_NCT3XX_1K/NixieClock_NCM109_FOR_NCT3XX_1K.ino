 const String FirmwareVersion = "015900";
#define HardwareVersion "MCU109 for 3XX_1K Series."
//Format                _X.XX__
//NIXIE CLOCK NCM109 3xx v1.0 by GRA & AFCH (fominalec@gmail.com)
//1.59 25.01.2025
//Added: Night Mode
//1.56 05.09.2023
//Added: RV-3028-C7 RTC support
//1.55 03.02.2023 
//Fixed: DS18b20 zeros bug
//1.54 15.05.2020 
// The driver has been changed to support BOTH HV5122 and HV 5222 (switching using resistor R5222 Arduino pin No. 8)
//SPI initialization moved to SPI_Init()
//1.53 09.04.2020
//Dots sync with seconds
//1.88 26.03.2020
//1.52 01.24.2020
//Added: DS3231 internal temperature sensor self test: 5 beeps if fail.
//1.5 04.08.2018
//Added: Dual Date Format
//1.41 04.08.2018
//Fixed: temp reading speed
//Fixed: RGB LEDs color reading from EEPROM
//Fixed: dot mixed up (Driver v1.2 is required)
//1.4 11.07.2018
//Added: Temperature reading 
//1.033 Self test bugfix
//1.032 
//Added: time sync with RTC each 60 seconds
//1.02 17.10.2016
//Fixed: RGB color controls
//Update to Arduino IDE 1.6.12 (Time.h replaced to TimeLib.h)
//1.01
//Added RGB LEDs lock(by UP and Down Buttons)
//Added Down and Up buttons pause and resume self testing

//#define tubes8
#define tubes6
//#define tubes4

#include <SPI.h>
#include <Wire.h>
#include <ClickButton.h>
#include <TimeLib.h>
#include <Tone.h>
#include <EEPROM.h>
#include "doIndication318_1K.h"
#include <OneWire.h>

boolean UD, LD; // DOTS control;

byte data[12];
byte addr[8];
int celsius, fahrenheit;

//const byte LEpin=10; //pin Latch Enabled data accepted while HI level
const byte HIZpin = 8; //pin Z state in registers outputs (while LOW level)
const byte RedLedPin = 9; //MCU WDM output for red LEDs 9-g
const byte GreenLedPin = 6; //MCU WDM output for green LEDs 6-b
const byte BlueLedPin = 3; //MCU WDM output for blue LEDs 3-r
const byte pinSet = A0;
const byte pinUp = A2;
const byte pinDown = A1;
const byte pinBuzzer = 2;
const byte pinUpperDots = 12; //HIGH value light a dots
const byte pinLowerDots = 8; //HIGH value light a dots
//const uint16_t fpsLimit = 16666; // 1/60*1.000.000 //limit maximum refresh rate on 60 fps
bool RTC_present;
#define US_DateFormat 1
#define EU_DateFormat 0

const byte pinTemp = A3;
OneWire  ds(pinTemp);
String TempDegrees;
bool TempPresent = false;
#define CELSIUS 0
#define FAHRENHEIT 1

//word SymbolArray[];

bool NightMode = false;
bool RGBLedsStateBeforeNightMode = false;

String stringToDisplay = "000000";// Conten of this string will be displayed on tubes (must be 6 chars length)
int menuPosition = 0; // 0 - time
// 1 - date
// 2 - alarm
// 3 - 12/24 hours mode
// 4 - Temperature
// 5 - Night Mode

int blinkMask = B00000000; //bit mask for blinkin digits (1 - blink, 0 - constant light)
int blankMask = B00000000; //bit mask for digits (1 - off, 0 - on)

byte dotPattern = B00000000; //bit mask for separeting dots (1 - on, 0 - off) 
//B10000000 - upper dots
//B01000000 - lower dots

#define DS1307_ADDRESS  0x68 //DS3231 
#define RV_3028_ADDRESS 0x52 //RV-3028-C7
uint8_t RTC_Address=DS1307_ADDRESS;

byte zero = 0x00; //workaround for issue #527
int RTC_hours, RTC_minutes, RTC_seconds, RTC_day, RTC_month, RTC_year, RTC_day_of_week;

#define TimeIndex           0
#define DateIndex           1
#define AlarmIndex          2
#define hModeIndex          3
#define TemperatureIndex    4
#define NightModeIndex      5  
#define TimeHoursIndex      6
#define TimeMintuesIndex    7
#define TimeSecondsIndex    8
#define DateFormatIndex     9
#define DateDayIndex        10
#define DateMonthIndex      11
#define DateYearIndex       12
#define AlarmHourIndex      13
#define AlarmMinuteIndex    14
#define AlarmSecondIndex    15
#define Alarm01             16
#define hModeValueIndex     17
#define DegreesFormatIndex  18
#define TempAdjustIndex     19
#define OffHourIndex        20
#define OnHourIndex         21

#define FirstParent      TimeIndex
#define LastParent       NightModeIndex
#define SettingsCount    (OnHourIndex+1)
#define NoParent         0
#define NoChild          0

//-------------------------------------0-------------------1------------2-----------------3-----------------4----------------5------------6-----------7-------------8----------9-------------10-----------11------------12----------13------------14------------15------------16------------17----------------18------------------19-----------------20---------------21
//                            names:  Time,              Date,        Alarm,            12/24,         Temperature,      Night Mode     hours,     mintues,     seconds,   DateFormat,       day,       month,        year,        hour,        minute,       second,     alarm01,      hour_format,    DegreesFormat,         TempAdj,           OffHour,         OffHour         
//                                     1                   1            1                 1                 1                1            1           1             1          1              1           1             1           1             1             1             1             1                 1                   1                  1                1
int parent[SettingsCount] = {      NoParent,            NoParent,    NoParent,         NoParent,         NoParent,       NoParent,  TimeIndex+1, TimeIndex+1, TimeIndex+1, DateIndex+1, DateIndex+1, DateIndex+1, DateIndex+1, AlarmIndex+1, AlarmIndex+1, AlarmIndex+1, AlarmIndex+1, hModeIndex+1, TemperatureIndex+1, TemperatureIndex+1, NightModeIndex+1, NightModeIndex+1 }; // +1 !!!!!!!!!
int firstChild[SettingsCount] = {TimeHoursIndex, DateFormatIndex, AlarmHourIndex,   hModeValueIndex, DegreesFormatIndex, OffHourIndex,    0,          0,            0,      NoChild,          0,          0,            0,          0,            0,            0,            0,            0,              NoChild,          NoChild,           NoChild,          NoChild      };
int lastChild[SettingsCount] = { TimeSecondsIndex,  DateYearIndex,   Alarm01,       hModeValueIndex, TempAdjustIndex,    OnHourIndex,     0,          0,            0,      NoChild,          0,          0,            0,          0,            0,            0,            0,            0,              NoChild,          NoChild,           NoChild,          NoChild      };
int value[SettingsCount] = {           0,                  0,           0,                0,                0,               0,           0,          0,            0,   EU_DateFormat,       0,          0,            0,          0,            0,            0,            0,            24,               0,                 0,                 22,               8         };
int maxValue[SettingsCount] = {        0,                  0,           0,                0,                0,               0,           23,         59,           59,  US_DateFormat,       31,         12,           99,         23,           59,           59,           1,            24,           FAHRENHEIT,            99,                23,               23        };
int minValue[SettingsCount] = {        0,                  0,           0,                12,               0,               0,           00,         00,           00,  EU_DateFormat,       1,          1,            00,         00,           00,           00,           0,            12,             CELSIUS,            -99,                0,                0,        };
int blinkPattern[SettingsCount] = {
  B00000000, //0
  B00000000, //1
  B00000000, //2
  B00000000, //3
  B00000000, //4
  B00000000, //5
  B00000011, //6
  B00001100, //7
  B00110000, //8
  B00111111, //9
  B00000011, //10
  B00001100, //11
  B00110000, //12
  B00000011, //13
  B00001100, //14
  B00110000, //15
  B11000000, //16
  B00001100, //17
  B00111111, //18
  B00000011, //19
  B00000011, //20
  B00001100  //21
};

bool editMode = false;

long downTime = 0;
long upTime = 0;
const long settingDelay = 150;
bool BlinkUp = true;
bool BlinkDown = true;
unsigned long enteringEditModeTime = 0;
bool RGBLedsOn = true;
#define RGBLEDsEEPROMAddress  0
#define HourFormatEEPROMAddress 1
#define AlarmTimeEEPROMAddress 2 //3,4,5
#define AlarmArmedEEPROMAddress 6
#define LEDsLockEEPROMAddress 7
#define LEDsRedValueEEPROMAddress 8
#define LEDsGreenValueEEPROMAddress 9
#define LEDsBlueValueEEPROMAddress 10
#define DegreesFormatEEPROMAddress 11
#define TempAdjustEEPROMAddress 12 //and 13
#define DateFormatEEPROMAddress 14
#define OffHourEEPROMAddress 15
#define OnHourEEPROMAddress 16

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
bool modeChangedByUser=false;
bool transactionInProgress=false; 
#define timeModePeriod 60000
#define dateModePeriod 5000
long modesChangePeriod=timeModePeriod;
  #ifdef tubes8
  const byte tubesQty=8;
  #endif
  #ifdef tubes6
  const byte tubesQty=6;
  #endif
  #ifdef tubes4
  const byte tubesQty=4;
  #endif
//*antipoisoning transaction 
/*******************************************************************************************************
  Init Programm
*******************************************************************************************************/
void setup()
{
  stringToDisplay.reserve(8);
  Wire.begin();
  //setRTCDateTime(23,40,00,25,7,15,1);

  Serial.begin(115200);

#ifdef tubes8
  maxValue[DateYearIndex] = 2099;
  minValue[DateYearIndex] = 2017;
  value[DateYearIndex] = 2017;
  blinkPattern[DateYearIndex] = B11110000;
#endif

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
  if (EEPROM.read(TempAdjustEEPROMAddress) == 255) value[TempAdjustIndex] = 0; else value[TempAdjustIndex] = tInt;
  if (EEPROM.read(DateFormatEEPROMAddress) != 255) value[DateFormatIndex] = EEPROM.read(DateFormatEEPROMAddress);
  if (EEPROM.read(OffHourEEPROMAddress) != 255) value[OffHourIndex] = EEPROM.read(OffHourEEPROMAddress);
  if (EEPROM.read(OnHourEEPROMAddress) != 255) value[OnHourIndex] = EEPROM.read(OnHourEEPROMAddress);

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

  if ( !ds.search(addr))
  {
    Serial.println(F("Temp. sensor not found."));
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
  
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  doTest();
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  if (LEDsLock == 1)
  {
    setLEDsFromEEPROM();
  }
  getRTCTime();
  byte prevSeconds=RTC_seconds;
  unsigned long RTC_ReadingStartTime=millis();
  RTC_present=true;
  while(prevSeconds==RTC_seconds)
  {
    getRTCTime();
    if ((millis()-RTC_ReadingStartTime)>3000)
    {
      Serial.println(F("Warning! RTC DON'T RESPOND!"));
      RTC_present=false;
      break;
    }
  }
  setTime(RTC_hours, RTC_minutes, RTC_seconds, RTC_day, RTC_month, RTC_year);
  setRTCDateTime(RTC_hours, RTC_minutes, RTC_seconds, RTC_day, RTC_month, RTC_year, 1); //записываем только что считанное время в RTC чтобы запустить новую микросхему
}

int rotator = 0; //index in array with RGB "rules" (increse by one on each 255 cycles)
int cycle = 0; //cycles counter
int RedLight = 255;
int GreenLight = 0;
int BlueLight = 0;
unsigned long prevTime = 0; // time of lase tube was lit
unsigned long prevTime4FireWorks = 0; //time of last RGB changed
//int minuteL=0; //младшая цифра минут

/***************************************************************************************************************
  MAIN Programm
***************************************************************************************************************/
void loop() 
{
  CheckNightMode();
  if (((millis()%60000)==0)&&(RTC_present)) //synchronize with RTC every 10 seconds
  {
    getRTCTime();
    setTime(RTC_hours, RTC_minutes, RTC_seconds, RTC_day, RTC_month, RTC_year);
    Serial.println(F("sync"));
  }

  p = playmusic(p);

  if ((millis() - prevTime4FireWorks) > 5)
  {
    rotateFireWorks(); //change color (by 1 step)
    prevTime4FireWorks = millis();
  }

  if ((menuPosition==TimeIndex) || (modeChangedByUser==false) ) modesChanger();
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
    modeChangedByUser=true;
    p = 0; //shut off music )))
    tone1.play(1000, 100);
    if (NightMode) 
    {
      ExitFromNightMode();
      return;
    }
    enteringEditModeTime = millis();
    menuPosition = menuPosition + 1;
    if (menuPosition == LastParent + 1) menuPosition = TimeIndex;
    // Serial.print(F("menuPosition="));
    // Serial.println(menuPosition);
    // Serial.print(F("value="));
    // Serial.println(value[menuPosition]);

    blinkMask = blinkPattern[menuPosition];
    if ((parent[menuPosition - 1] != 0) and (lastChild[parent[menuPosition - 1] - 1] == (menuPosition - 1)))
    {
      if ((parent[menuPosition - 1] - 1 == 1) && (!isValidDate()))
      {
        menuPosition = DateDayIndex;
        return;
      }
      editMode = false;
      menuPosition = parent[menuPosition - 1] - 1;
      if (menuPosition == TimeIndex) setTime(value[TimeHoursIndex], value[TimeMintuesIndex], value[TimeSecondsIndex], day(), month(), year());
#ifdef tubes8
      if (menuPosition == DateIndex) setTime(hour(), minute(), second(), value[DateDayIndex], value[DateMonthIndex], value[DateYearIndex]);
#endif
#ifdef tubes6
      if (menuPosition == DateIndex) 
      {
        setTime(hour(), minute(), second(), value[DateDayIndex], value[DateMonthIndex], 2000 + value[DateYearIndex]);
        EEPROM.write(DateFormatEEPROMAddress, value[DateFormatIndex]);
      }
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
        EEPROM.put(TempAdjustEEPROMAddress, value[TempAdjustIndex]);
      }
      if (menuPosition == NightModeIndex) 
      {
        EEPROM.write(OffHourEEPROMAddress, value[OffHourIndex]);
        EEPROM.write(OnHourEEPROMAddress, value[OnHourIndex]);
      }
      setRTCDateTime(hour(), minute(), second(), day(), month(), year() % 1000, 1);
      return;
    } //end exit from edit mode
    if ((menuPosition != TempAdjustIndex) &&
        (menuPosition != DegreesFormatIndex) &&
        (menuPosition != DateFormatIndex) &&
        (menuPosition != DateDayIndex))
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
    if (menuPosition == DateIndex) 
    {
      value[DateDayIndex] =  day();
      value[DateMonthIndex] = month();
      value[DateYearIndex] = year() % 1000;
      if (value[DateFormatIndex] == EU_DateFormat) stringToDisplay=PreZero(value[DateDayIndex])+PreZero(value[DateMonthIndex])+PreZero(value[DateYearIndex]);
        else stringToDisplay=PreZero(value[DateMonthIndex])+PreZero(value[DateDayIndex])+PreZero(value[DateYearIndex]);
    }
    menuPosition = firstChild[menuPosition];
    if (menuPosition == AlarmHourIndex) {
      value[Alarm01] = 1; dotPattern = B10000000;
    }
    editMode = !editMode;
    blinkMask = blinkPattern[menuPosition];
    if ((menuPosition != DegreesFormatIndex) &&
       (menuPosition != DateFormatIndex))
      value[menuPosition] = extractDigits(blinkMask);
  }

  if (upButton.clicks != 0) functionUpButton = upButton.clicks;

  if (upButton.clicks > 0)
  {
    modeChangedByUser=true;
    p = 0; //shut off music )))
    tone1.play(1000, 100);
    if (NightMode) 
    {
      ExitFromNightMode();
      return;
    }
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
    modeChangedByUser=true;
    p = 0; //shut off music )))
    tone1.play(1000, 100);
    if (NightMode) 
    {
      ExitFromNightMode();
      return;
    }
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
      setLEDsFromEEPROM();
    }
    if (downButton.clicks < 0)
    {
      tone1.play(1000, 100);
      RGBLedsOn = false;
      EEPROM.write(RGBLEDsEEPROMAddress, 0);
    }
  }

  static bool updateDateTime = false;
  switch (menuPosition)
  {
    case TimeIndex: //time mode
      if (!transactionInProgress) stringToDisplay=updateDisplayString();
      doDotBlink();
      checkAlarmTime();
      break;
    case DateIndex: //date mode
      if (!transactionInProgress) stringToDisplay=updateDateString();
      dotPattern = B01000000; //turn on lower dots
      checkAlarmTime();
      break;
    case AlarmIndex: //alarm mode
#ifdef tubes8
      stringToDisplay = PreZero(value[AlarmHourIndex]) + PreZero(value[AlarmMinuteIndex]) + PreZero(value[AlarmSecondIndex]) + "00";
#endif
#ifdef tubes6
      stringToDisplay = PreZero(value[AlarmHourIndex]) + PreZero(value[AlarmMinuteIndex]) + PreZero(value[AlarmSecondIndex]);
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
#endif
#ifdef tubes6
      stringToDisplay = String(F("00")) + String(value[hModeValueIndex]) + F("00");
#endif
      dotPattern = B00000000; //turn off all dots
      checkAlarmTime();
      break;
    case TemperatureIndex: //missed break
    case DegreesFormatIndex:
      if (!transactionInProgress)
      {
        stringToDisplay = updateTemperatureString(getTemperature(value[DegreesFormatIndex]));
        if (value[DegreesFormatIndex] == CELSIUS)
        {
          blankMask = B11000111;
          dotPattern = B01000000;
        }
        else
        {
          blankMask = B10001111;
          dotPattern = B00000000;
        }
      }

      if (getTemperature(value[DegreesFormatIndex]) < 0) dotPattern |= B10000000;
       else dotPattern &= B01111111;
      break;
    case TempAdjustIndex:
    stringToDisplay = updateTemperatureString(getTemperature(value[DegreesFormatIndex]));
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
    case DateFormatIndex:
      if (value[DateFormatIndex] == EU_DateFormat) 
      {
        stringToDisplay=F("311299");
        blinkPattern[DateDayIndex]=B00000011;
        blinkPattern[DateMonthIndex]=B00001100;
      }
        else 
        {
          stringToDisplay=F("123199");
          blinkPattern[DateDayIndex]=B00001100;
          blinkPattern[DateMonthIndex]=B00000011;
        }
     break; 
    case DateDayIndex:
    case DateMonthIndex:
    case DateYearIndex:
      if (value[DateFormatIndex] == EU_DateFormat) stringToDisplay=PreZero(value[DateDayIndex])+PreZero(value[DateMonthIndex])+PreZero(value[DateYearIndex]);
        else stringToDisplay=PreZero(value[DateMonthIndex])+PreZero(value[DateDayIndex])+PreZero(value[DateYearIndex]);
     break;
    case NightModeIndex:
    case OffHourIndex:
    case OnHourIndex:
      stringToDisplay = PreZero(value[OffHourIndex]) + PreZero(value[OnHourIndex]) + F("01");
      dotPattern = B00000000; //turn off all dots
    break;
  }
}

String PreZero(int digit)
{
  if (digit < 10) return String(F("0")) + String(digit);
  else return String(digit);
}

void rotateFireWorks()
{
  if (!RGBLedsOn)
  {
    analogWrite(RedLedPin, 0 );
    analogWrite(GreenLedPin, 0);
    analogWrite(BlueLedPin, 0);
    return;
  }
  if (LEDsLock) return;
  RedLight = RedLight + fireforks[rotator * 3];
  GreenLight = GreenLight + fireforks[rotator * 3 + 1];
  BlueLight = BlueLight + fireforks[rotator * 3 + 2];
  analogWrite(RedLedPin, RedLight );
  analogWrite(GreenLedPin, GreenLight);
  analogWrite(BlueLedPin, BlueLight);
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
  static int prevS=-1;

  if (second()!=prevS)
  {
    prevS=second();
    return getTimeNow();
  } else return stringToDisplay;
}

String getTimeNow()
{
  #ifdef tubes8
    if (value[hModeValueIndex] == 24) return PreZero(hour()) + PreZero(minute()) + PreZero(second()) + (millis() % 1000) / 100 + "0";
    else return PreZero(hourFormat12()) + PreZero(minute()) + PreZero(second());
#endif
#ifdef tubes6
    if (value[hModeValueIndex] == 24) return PreZero(hour()) + PreZero(minute()) + PreZero(second());
    else return PreZero(hourFormat12()) + PreZero(minute()) + PreZero(second());
#endif
}

void doTest()
{
  Serial.print(F("Firmware version: "));
  Serial.println(FirmwareVersion.substring(1,2)+"."+FirmwareVersion.substring(2,5));
  Serial.println(HardwareVersion);
  Serial.println(F("Start Test"));
  
  p=song;
  parseSong(p);
   
  analogWrite(RedLedPin,255);
  delay(1000);
  analogWrite(RedLedPin,0);
  analogWrite(GreenLedPin,255);
  delay(1000);
  analogWrite(GreenLedPin,0);
  analogWrite(BlueLedPin,255);
  delay(1000); 

  // Serial.print("Free ram = ");
  // Serial.println(freeRam());
  #ifdef tubes8
  String testStringArray[11]={"00000000","11111111","22222222","33333333","44444444","55555555","66666666","77777777","88888888","99999999",""};
  testStringArray[10]=FirmwareVersion+"00";
  #endif
  #ifdef tubes6
  String testStringArray[11] = {F("000000"), F("111111"), F("222222"), F("333333"), F("444444"), F("555555"), F("666666"), F("777777"), F("888888"), F("999999"), ""};
  testStringArray[10]=FirmwareVersion;
  #endif
  //testStringArray[12]="00"+PreZero(celsius)+"00";
  Serial.print(F("Temp = "));
  Serial.println(celsius);

  int dlay=500;
  bool test=1;
  byte strIndex=-1;
  unsigned long startOfTest=millis()+1000; //disable delaying in first iteration
  bool digitsLock=false;
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
       Serial.println(stringToDisplay);
      }
    doIndication();  
  }

  RTC_Test();
  //Serial.println(F("Stop Test"));
}

void doDotBlink()
{
  if (second()%2 == 0) dotPattern = B11000000;
    else dotPattern = B00000000;
}

void setRTCDateTime(byte h, byte m, byte s, byte d, byte mon, byte y, byte w)
{
  Wire.beginTransmission(RTC_Address);
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
  Wire.beginTransmission(RTC_Address);
  Wire.write(zero);
  Wire.endTransmission();

  Wire.requestFrom(RTC_Address, 7);

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
  if (b == B00000011) stringToDisplay = PreZero(value) + stringToDisplay.substring(2);
  if (b == B00001100) stringToDisplay = stringToDisplay.substring(0, 2) + PreZero(value) + stringToDisplay.substring(4);
  if (b == B00110000) stringToDisplay = stringToDisplay.substring(0, 4) + PreZero(value);
  if (b == B11110000) stringToDisplay = stringToDisplay.substring(0, 4) + PreZero(value);
  if (b == B11000000) stringToDisplay = stringToDisplay.substring(0, 6) + PreZero(value);
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
    if (menuPosition!=DateFormatIndex) injectDigits(blinkMask, value[menuPosition]);
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
    if (menuPosition!=DateFormatIndex) injectDigits(blinkMask, value[menuPosition]);
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
    ExitFromNightMode();
    lastTimeAlarmTriggired = millis();
    Alarm1SecondBlock = true;
    Serial.println(F("Wake up, Neo!"));
    p = song;
  }
}

int freeRam()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void setLEDsFromEEPROM()
{
  if (RGBLedsOn)
  {
    analogWrite(RedLedPin, EEPROM.read(LEDsRedValueEEPROMAddress));
    analogWrite(GreenLedPin, EEPROM.read(LEDsGreenValueEEPROMAddress));
    analogWrite(BlueLedPin, EEPROM.read(LEDsBlueValueEEPROMAddress));
  } else
  {
    digitalWrite(RedLedPin, LOW);
    digitalWrite(GreenLedPin, LOW);
    digitalWrite(BlueLedPin, LOW);
  }
}

void modesChanger()
{
  if (editMode == true) return;
  static unsigned long lastTimeModeChanged = millis();
  static unsigned long lastTimeAntiPoisoningIterate = millis();
  static int transnumber = 0;
  if ((millis() - lastTimeModeChanged) > modesChangePeriod)
  {
    lastTimeModeChanged = millis();
    if (transnumber == 0) {
      menuPosition = DateIndex;
      modesChangePeriod = dateModePeriod;
    }
    if (transnumber == 1) {
      menuPosition = TemperatureIndex;
      modesChangePeriod = dateModePeriod;
      if (!TempPresent) transnumber = 2;
    }
    if (transnumber == 2) {
      menuPosition = TimeIndex;
      modesChangePeriod = timeModePeriod;
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
      if (TempPresent)
      {
        if (menuPosition == TimeIndex) stringToDisplay = antiPoisoning2(updateTemperatureString(getTemperature(value[DegreesFormatIndex])), getTimeNow());
        if (menuPosition == DateIndex) stringToDisplay = antiPoisoning2(getTimeNow(), PreZero(day()) + PreZero(month()) + PreZero(year() % 1000));
        if (menuPosition == TemperatureIndex) stringToDisplay = antiPoisoning2(PreZero(day()) + PreZero(month()) + PreZero(year() % 1000), updateTemperatureString(getTemperature(value[DegreesFormatIndex])));
      } else
      {
        if (menuPosition == TimeIndex) stringToDisplay = antiPoisoning2(PreZero(day()) + PreZero(month()) + PreZero(year() % 1000), getTimeNow());
        if (menuPosition == DateIndex) stringToDisplay = antiPoisoning2(getTimeNow(), PreZero(day()) + PreZero(month()) + PreZero(year() % 1000));
      }
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
  static byte iterationCounter=0;
  if (!transactionInProgress) 
  {
    transactionInProgress=true;
    for (int i=0; i<tubesQty; i++)
    {
      currentDigits[i]=fromStr.substring(i, i+1).toInt();
      toDigits[i]=toStr.substring(i, i+1).toInt();
    }
  }
  for (int i=0; i<tubesQty; i++)
  {
    if (iterationCounter<10) currentDigits[i]++;
      else if (currentDigits[i]!=toDigits[i]) currentDigits[i]++;
    if (currentDigits[i]==10) currentDigits[i]=0;
  }
  iterationCounter++;
  if (iterationCounter==20)
  {
    iterationCounter=0;
    transactionInProgress=false;
  }
  String tmpStr;
  for (int i=0; i<tubesQty; i++)
    tmpStr+=currentDigits[i];
  return tmpStr;
}

String updateDateString()
{
  static unsigned long lastTimeDateUpdate = millis()+1001;
  static String DateString; // = PreZero(day()) + PreZero(month()) + PreZero(year() % 1000);
  DateString.reserve(8);
  static byte prevoiusDateFormatWas=value[DateFormatIndex];
  if (((millis() - lastTimeDateUpdate) > 1000) || (prevoiusDateFormatWas != value[DateFormatIndex]))
  {
    lastTimeDateUpdate = millis();
    if (value[DateFormatIndex]==EU_DateFormat) DateString = PreZero(day()) + PreZero(month()) + PreZero(year() % 1000);
      else DateString = PreZero(month()) + PreZero(day()) + PreZero(year() % 1000);
  }
  return DateString;
}

String updateTemperatureString(float fDegrees)
{
  static  unsigned long lastTimeTemperatureString=millis()+1100;
  static String strTemp; // = "000000";
  strTemp.reserve(8);
  if ((millis() - lastTimeTemperatureString) > 1000)
  {
    lastTimeTemperatureString = millis();
    int iDegrees = abs(round(fDegrees));
    if (value[DegreesFormatIndex] == CELSIUS)
    {
      if (iDegrees > 1000) strTemp = String(F("0")) + String(iDegrees) + F("0");
        else if (iDegrees > 100) strTemp = String(F("00")) + String(iDegrees) + F("0");
          else if (iDegrees > 10) strTemp = String(F("000")) + String(iDegrees) + F("0");
            else strTemp = String(F("0000")) + String(iDegrees) + F("0");
    }else
    {
      iDegrees = iDegrees / 10;
      if (iDegrees > 1000) strTemp = String(iDegrees) + F("00");
        else if (iDegrees > 100) strTemp = String(F("0")) + String(iDegrees) + F("00");
          else if (iDegrees > 10) strTemp = String(F("00")) + String(iDegrees) + F("00");
            else strTemp = String(F("000")) + String(iDegrees) + F("00");
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

  static uint32_t startTime=millis();

  switch (iterator) 
  {
    case 0: ds.reset(); break; // 1 ms
    case 1: ds.write(0xCC); break; //
    case 2: ds.write(0x44); startTime=millis(); break; // 0-1 ms
    case 3: if (millis()-startTime < 750) return fDegrees; break;
    case 4: ds.reset(); break; //1 ms
    case 5: ds.write(0xCC); break; //
    case 6: ds.write(0xBE); break; //send request to all devices
    case 7: TempRawData[0] = ds.read(); break;
    case 8: TempRawData[1] = ds.read(); break;
    default:  break;
  }
  
 if (iterator == 9)
  {
    int16_t raw = (TempRawData[1] << 8) | TempRawData[0];
    if (raw == -1) raw = 0;
    float celsius = (float)raw / 16.0;
    celsius = celsius + (float)value[TempAdjustIndex]/10;//users adjustment
  
    if (!bTempFormat) fDegrees = celsius * 10;
    else fDegrees = (celsius * 1.8 + 32.0) * 10;
  }
  iterator++;
  if (iterator==10) iterator=0;
  return fDegrees;
}

void RTC_Test()
{
  uint8_t errorCounter = 0;
  int8_t DS3231InternalTemperature = 0;
  Wire.beginTransmission(RTC_Address);
  Wire.write(0x11);
  Wire.endTransmission();

  Wire.requestFrom(RTC_Address, 2);
  DS3231InternalTemperature = Wire.read();
  if ((DS3231InternalTemperature < 5) || (DS3231InternalTemperature > 60))
  {
    errorCounter++;
    RTC_Address = RV_3028_ADDRESS;
  }

  Wire.beginTransmission(RTC_Address);
  Wire.write(0x28);
  Wire.endTransmission();

  Wire.requestFrom(RTC_Address, 1);

  if (Wire.read() <= 0)
  {
    errorCounter++;
  }

  if (errorCounter == 2)
  {
    Serial.println(F("Faulty RTC!"));
    for (int i = 0; i < 5; i++)
    {
      tone1.play(1000, 1000);
      delay(2000);
    }
    return;
  }

  Wire.beginTransmission(RTC_Address);
  Wire.write(0x0F);
  Wire.write(0x08);
  Wire.endTransmission(); //disable auto refresh

  Wire.beginTransmission(RTC_Address);
  Wire.write(0x37);
  Wire.write(0x1C);
  Wire.endTransmission();//Level Switching Mode

  Wire.beginTransmission(RTC_Address);
  Wire.write(0x27);
  Wire.write(0x00);
  Wire.endTransmission();//Update EEPROM
  Wire.beginTransmission(RTC_Address);
  Wire.write(0x27);
  Wire.write(0x11);
  Wire.endTransmission();//Update EEPROM

  Wire.beginTransmission(RTC_Address);
  Wire.write(0x0F);
  Wire.write(0x00);
  Wire.endTransmission(); //enable auto refresh

  Wire.beginTransmission(RTC_Address);
  Wire.write(0x0E);
  Wire.write(0x00);
  Wire.endTransmission(); //reset RTC
}

void CheckNightMode()
{
  static uint8_t prevHour = hour();

  if (editMode == true) return;

  if (prevHour != hour()) 
  {
    prevHour = hour();
    if (hour() == value[OffHourIndex]) 
    {
      NightMode = true;
      RGBLedsStateBeforeNightMode = RGBLedsOn;
      //Serial.print(F("RGBLedsOn="));
      //Serial.print(RGBLedsOn);
      RGBLedsOn = false;
      //Serial.println(F("NightMode ON"));
    }
    if (hour() == value[OnHourIndex]) 
    {
      //NightMode = false;
      //RGBLedsOn = RGBLedsStateBeforeNightMode;
      ExitFromNightMode();
    }
  }
}

void ExitFromNightMode()
{
  if (NightMode)
  {
    NightMode = false;
    RGBLedsOn = RGBLedsStateBeforeNightMode;
    //Serial.print(F("RGBLedsOn="));
    //Serial.print(RGBLedsOn);
    setLEDsFromEEPROM();
  }
}