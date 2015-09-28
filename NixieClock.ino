/*
Nixie Clock 
ver 14.09.2015
добавлен автовыход из режима настройки через 1 минуту
во время автоинкремента отключается мигание
долгое нажатие кнопок Up и Down включает или отключает RGB подсветку
ver 13.09.2015
реализована настройка даты и времени с кнопок
введена функция проверки корректности даты
и функции изменения строки вывода поразрядно
а также автоинкремент/дикремент при удержании кнопок вверх-вниз
*/


/*
Nixie Clock 
ver 10.09.2015
переписана функция мигания разрядами doEditBlink
добавлена структура меню
*/
/*
Nixie Clock 
ver 04.09.2015
добавлена функция мигания разрядами doEditBlink
*/

#include <SPI.h>
#include <Wire.h>
#include <ClickButton.h>
#include <Time.h>

int dataPin=11; //Ножка данныз для регистров
int clockPin=13; //ножка такитрования регистров
int LEpin=7; //Ножка Latch Enabled данные регистром принимаются когда уровень равен 1
int HIZpin=8; //Ножка активации Z состояния на выходах регистров, если равна 0
int DHVpin=5; // off/on MAX1771 Driver  Hight Voltage(DHV) 110-220V 
int RedLedPin=9; //выводы МК с ШИМ для управления цветами RGB светодиодов
int GreenLedPin=6; //выводы МК с ШИМ для управления цветами RGB светодиодов
int BlueLedPin=3; //выводы МК с ШИМ для управления цветами RGB светодиодов
int pinSet=A2;
int pinUp=A1;
int pinDown=A0;
int pinBuzzer=2;
String stringToDisplay="000000";// Строка которая будет выводится на лампы (должна быть длинной 6 цифр, причем пробел обозначает погашенную цифру)
int menuPosition=0; // 0 - время
                    // 1 - дата
                    // 2 - будильник(00:00:00) выключенный будильник
                    
byte blinkMask=B00000000; //маска которая позволит мигать (парами) цифр которые можно настраивать 0 - не мигаем, 1 - мигаем.



//-------------------------0-------1----------2----------3---------4--------5---------6---------7---------8---------9-----
byte lowBytesArray[]={B11111110,B11111101,B11111011,B11110111,B11101111,B11011111,B10111111,B01111111,B11111111,B11111111};
byte highBytesArray[]={B11111111,B11111111,B11111111,B11111111,B11111111,B11111111,B11111111,B11111111,B11111110,B11111101};

byte dotPattern=B00000000; //битовая маска для включения или выключения мигающих разделительны точек два старших разряда включают или отключают
                          //B00000000 - выключает обе точки и верхние и нижние
                          //B1100000 - включает

#define DS1307_ADDRESS 0x68
byte zero = 0x00; //workaround for issue #527
int RTC_hours, RTC_minutes, RTC_seconds, RTC_day, RTC_month, RTC_year, RTC_day_of_week;

//--    ------------0--------1--------2--------3--------4--------5--------6--------7--------8--------9-------10-------11
//         names:  Time,   Date,   Alarm,   hours,   mintues, seconds,  day,    month,   year,    hour,   minute,   second
//                  1        1        1        1        1        1        1        1        1        1        1        1
int parent[12]={    0,       0,       0,       1,       1,       1,       2,       2,       2,       3,       3,       3};
int firstChild[12]={3,       6,       9,       0,       0,       0,       0,       0,       0,       0,       0,       0};
int lastChild[12]={ 5,       8,       11,      0,       0,       0,       0,       0,       0,       0,       0,       0};
int value[12]={     0,       0,       0,       0,       0,       0,       0,       0,       0,       0,       0,       0};
int maxValue[12]={  0,       0,       0,      24,      59,      59,      31,      12,      99,      24,      59,      59};
byte blinkPattern[12]={B00000000, B00000000, B00000000, B00000011, B00001100, B00110000, B00000011, B00001100, B00110000, B00000011, B00001100, B00110000};
bool editMode=false;

long downTime=0;
long upTime=0;
const long settingDelay=150;
bool BlinkUp=false;
bool BlinkDown=false;
unsigned long enteringEditModeTime=0;
bool RGBLedsOn=true;

//объявление кнопок
ClickButton setButton(pinSet, LOW, CLICKBTN_PULLUP);
ClickButton upButton(pinUp, LOW, CLICKBTN_PULLUP);
ClickButton downButton(pinDown, LOW, CLICKBTN_PULLUP);
///////////////////

// Пробую написать свой класс для активации выхода на заднанное время
class PulseOut
{
 public:
   PulseOut(int pin, boolean initState);
   void Update();
   void Pulse(boolean state, unsigned long ms);
 private:
   int _pin;
   boolean _state;
   unsigned long _ms;
   unsigned long _time_of_start;
};

void PulseOut::Pulse(boolean state, unsigned long ms)
{
  _state=state;
  _ms=ms;
  _time_of_start=millis();
  digitalWrite(_pin, _state); 
}

PulseOut::PulseOut(int pin, boolean initState)
{
  _pin =pin;
  _state=!initState;
  digitalWrite(_pin, initState);
}

void PulseOut::Update()
{
  if ((millis()-_time_of_start)>_ms) digitalWrite(_pin, !_state);
}
//

PulseOut buzz(pinBuzzer, LOW);

int fireforks[]={0,0,1,
                -1,0,0,
                 0,1,0,
                 0,0,-1,
                 1,0,0,
                 0,-1,0}; //массив описывающий последовательность изменения цветов 0 - ничего, -1 - уменьшаем значение, +1 - увлеичиваем

uint8_t nomad = B11111110; // это наш бегающий бит

int matrix[]={    B11111110, //0
                  B11111101, //1
                  B11111011, //2
                  B11110111, //3
                  B11101111, //4
                  B11011111, //5
                  B10111111, //6
                  B01111111, //7
                  B01111111, //8
                  B01111111};//9

void setRTCDateTime(byte h, byte m, byte s, byte d, byte mon, byte y, byte w=1);

int functionDownButton=0;
int functionUpButton=0;

/*******************************************************************************************************
Init Programm
*******************************************************************************************************/
void setup() 
{
  digitalWrite(DHVpin, LOW);    // off MAX1771 Driver  Hight Voltage(DHV) 110-220V
  digitalWrite(HIZpin, LOW);    // High impedence outputs
  
  Wire.begin();
  //setRTCDateTime(23,40,00,25,7,15,1);
  
  Serial.begin(9600);
  
  pinMode(LEpin, OUTPUT);
  pinMode(HIZpin, OUTPUT);
  pinMode(DHVpin, OUTPUT);
  
  pinMode(RedLedPin, OUTPUT);
  pinMode(GreenLedPin, OUTPUT);
  pinMode(BlueLedPin, OUTPUT);
  
  
 /********************************************************************************
   Инициализируем шину SPI. Если используется программная реализация,
   то вы должны сами настроить пины, по которым будет работать SPI.
 ********************************************************************************/
  SPI.begin(); // 
  SPI.setDataMode (SPI_MODE3); // Mode 3 SPI
  SPI.setClockDivider(SPI_CLOCK_DIV128); // SCK = 16MHz/128= 125kHz
 
  //инициальизурем пины кнопок
  pinMode(pinSet,  INPUT_PULLUP);
  pinMode(pinUp,  INPUT_PULLUP);
  pinMode(pinDown,  INPUT_PULLUP);
  ////////////////////////////
  pinMode(pinBuzzer, OUTPUT);
  
  //инициализация кнопок
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
  digitalWrite(DHVpin, HIGH); // on MAX1771 Driver  Hight Voltage(DHV) 110-220V
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  //doTest();
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  getRTCTime();
  setTime(RTC_hours, RTC_minutes, RTC_seconds, RTC_day, RTC_month, RTC_year);
  digitalWrite(DHVpin, LOW); // off MAX1771 Driver  Hight Voltage(DHV) 110-220V
  setRTCDateTime(RTC_hours,RTC_minutes,RTC_seconds,RTC_day,RTC_month,RTC_year,1); //записываем только что считанное время в RTC чтобы запустить новую микросхему
  digitalWrite(DHVpin, HIGH); // on MAX1771 Driver  Hight Voltage(DHV) 110-220V
  
 
}

void rotateLeft(uint8_t &bits)
{
   uint8_t high_bit = bits & (1 << 7) ? 1 : 0;
  bits = (bits << 1) | high_bit;
}

int rotator=0; //индекс массива с "правилами" изменения цвета на каждой итерации увеличививется каждые 255 циклов
int cycle=0; //счетчик циклов
int RedLight=255;
int GreenLight=0;
int BlueLight=0;
unsigned long prevTime=0; // время последнего включения цифры на лампе
unsigned long prevTime4FireWorks=0;  //время последнего изменения цвета светодиодов
int minuteL=0; //младшая цифра минут

/***************************************************************************************************************
MAIN Programm
***************************************************************************************************************/
void loop() {
  
  //stringToDisplay=updateDisplayString(); // разкоментил 
  //doDynamicIndication();
   //Super_Test(); // ГРИАШ
  
  if ((millis()-prevTime4FireWorks)>5)
  {
    rotateFireWorks(); //меняем цвет (на 1 шаг)
    prevTime4FireWorks=millis();
  }
    
  // stringToDisplay=updateDisplayString();
  //doEditBlink(B00000011);
  doDynamicIndication();
  
  setButton.Update();
  upButton.Update();
  downButton.Update();
  if (editMode==false) 
    {
      blinkMask=B00000000;
      
    } else if ((millis()-enteringEditModeTime)>60000) 
    {
      editMode=false;
      menuPosition=firstChild[menuPosition];
      blinkMask=blinkPattern[menuPosition];
    }
  if (setButton.clicks>0) 
    {
      buzz.Pulse(HIGH,30);
      enteringEditModeTime=millis();
      menuPosition=menuPosition+1;
      if (menuPosition==2) menuPosition=0; //меню номер 3 для установок будильника - будет реализовано в версии 2.0
      
      blinkMask=blinkPattern[menuPosition];
      if (lastChild[parent[menuPosition-1]-1]==(menuPosition-1)) 
      {
        if ((parent[menuPosition-1]-1==1) && (!isValidDate())) 
          {
            menuPosition=6;
            return;
          }
        editMode=false;
        menuPosition=parent[menuPosition-1]-1;
        if (menuPosition==0) setTime(value[3], value[4], value[5], day(), month(), year());
        if (menuPosition==1) setTime(hour(), minute(), second(),value[6], value[7], 2000+value[8]);
        digitalWrite(DHVpin, LOW); // off MAX1771 Driver  Hight Voltage(DHV) 110-220V
        setRTCDateTime(hour(),minute(),second(),day(),month(),year()%1000,1);
        digitalWrite(DHVpin, HIGH); // on MAX1771 Driver  Hight Voltage(DHV) 110-220V
      }
      value[menuPosition]=extractDigits(blinkMask);
    }
  if (setButton.clicks<0) 
    {
      buzz.Pulse(HIGH,150);
      if (!editMode) enteringEditModeTime=millis();
      menuPosition=firstChild[menuPosition];
      editMode=!editMode;
      blinkMask=blinkPattern[menuPosition];
      value[menuPosition]=extractDigits(blinkMask);
    }
   
  if (upButton.clicks != 0) functionUpButton = upButton.clicks;  
   
  if (upButton.clicks>0) 
    {
      buzz.Pulse(HIGH,30);
      enteringEditModeTime=millis();
      if (editMode==true)
      {
      if (value[menuPosition]<maxValue[menuPosition])
        value[menuPosition]=value[menuPosition]+1;
        else value[menuPosition]=0;
      injectDigits(blinkMask, value[menuPosition]);
      }
    }
  
  if (functionUpButton == -1 && upButton.depressed == true)   
  {
   BlinkUp=false;
   enteringEditModeTime=millis();
   if (editMode==true) 
   {
     if ( (millis() - upTime) > settingDelay)
    {
     upTime = millis();// + settingDelay;
     //Serial.println("DOwn");
     if (value[menuPosition]<maxValue[menuPosition])
        value[menuPosition]=value[menuPosition]+1;
        else value[menuPosition]=0;
      injectDigits(blinkMask, value[menuPosition]);
    }
   }
  } else BlinkUp=true;
  
  if (downButton.clicks != 0) functionDownButton = downButton.clicks;
  
  if (downButton.clicks>0) 
    {
      buzz.Pulse(HIGH,30);
      enteringEditModeTime=millis();
      if (editMode==true)
      {
      if (value[menuPosition]>0)
        value[menuPosition]=value[menuPosition]-1;
        else value[menuPosition]=maxValue[menuPosition];
      injectDigits(blinkMask, value[menuPosition]);
      }
    }
  
  if (functionDownButton == -1 && downButton.depressed == true)   
  {
   BlinkDown=false;
   enteringEditModeTime=millis(); 
   if (editMode==true) 
   {
     if ( (millis() - downTime) > settingDelay)
    {
     downTime = millis();// + settingDelay;
     //Serial.println("DOwn");
     if (value[menuPosition]>0)
        value[menuPosition]=value[menuPosition]-1;
        else value[menuPosition]=maxValue[menuPosition];
      injectDigits(blinkMask, value[menuPosition]);
    }
   }
  } else BlinkDown=true;
  
  //if ((upButton.depressed==false)||(upButton.depressed==false)) Blink=true;
  if (!editMode)
  {
    if (upButton.clicks<0)
      {
        buzz.Pulse(HIGH,30);
        RGBLedsOn=true;
        Serial.println("RGB=on");
      }
    if (downButton.clicks<0)
    {
      buzz.Pulse(HIGH,30);
      RGBLedsOn=false;
      Serial.println("RGB=off");
    }
  }
  
    
  buzz.Update();
  static bool updateDateTime=false;
 /* if ((hour()==0) && (minute()==0) && (second()==0))
       {
        if (updateDateTime==false)
        {
        digitalWrite(DHVpin, LOW);    // off MAX1771 Driver  Hight Voltage(DHV) 110-220V
        digitalWrite(HIZpin, LOW);    // High impedence outputs 
        getRTCTime();
        setTime(RTC_hours, RTC_minutes, RTC_seconds, RTC_day, RTC_month, RTC_year);
        digitalWrite(DHVpin, HIGH);    // off MAX1771 Driver  Hight Voltage(DHV) 110-220V
        digitalWrite(HIZpin, LOW);    // High impedence outputs 
        updateDateTime==true;
        }
       } else {updateDateTime==false;}*/
  switch (menuPosition)
  {
    case 0: 
      //stringToDisplay=PreZero(RTC.hour)+String(":")+PreZero(RTC.minute)+String(":")+PreZero(RTC.second);
       stringToDisplay=updateDisplayString();
       doDotBlink();
       break;
    case 1: 
      //stringToDisplay=String(RTC.year).substring(2)+String(".")+PreZero(RTC.month)+String(".")+PreZero(RTC.day);
      stringToDisplay=PreZero(day())+PreZero(month())+PreZero(year()%1000);
      dotPattern=B11000000;//включаем все точки
      break;
    case 2: 
      stringToDisplay="00 00 00";
      dotPattern=B00000000;//выключаем все точки
      break;
  }
  
}

/***************************************************************************************************************
Grisha Super Test
***************************************************************************************************************/
void Super_Test(void)
{
  while(1)
  {
   doDynamicIndication(); 
  }
}


String PreZero(int digit)
{
  if (digit<10) return String("0")+String(digit);
    else return String(digit);
}

void rotateFireWorks()
{
  if (!RGBLedsOn)
  {
    analogWrite(RedLedPin,0 );
    analogWrite(GreenLedPin,0);
    analogWrite(BlueLedPin,0); 
  }
  RedLight=RedLight+fireforks[rotator*3];
  GreenLight=GreenLight+fireforks[rotator*3+1];
  BlueLight=BlueLight+fireforks[rotator*3+2];
  analogWrite(RedLedPin,RedLight );
  analogWrite(GreenLedPin,GreenLight);
  analogWrite(BlueLedPin,BlueLight);  
  cycle=cycle+1;
  if (cycle==255)
  {  
    rotator=rotator+1;
    cycle=0;
  }
  if (rotator>5) rotator=0;
}


void doDynamicIndication()
{
  static byte b=B00000001;

  static  unsigned long lastTimeInterval1Started;
  if ((micros()-lastTimeInterval1Started)>3000)
  {
    lastTimeInterval1Started=micros();
    //делаем что-то полезное тут
    digitalWrite(HIZpin, LOW);    // High impedence outputs
    digitalWrite(LEpin, HIGH);    // allow data input (Transparent mode)
   
    int curTube=int(log(b)/log(2));
   
    SPI.transfer((b|dotPattern) & doEditBlink()); // anode
    SPI.transfer(highBytesArray[stringToDisplay.substring(curTube,curTube+1).toInt()]);
    SPI.transfer(lowBytesArray[stringToDisplay.substring(curTube,curTube+1).toInt()]);
      
    digitalWrite(LEpin, LOW);     // latching data 
    digitalWrite(HIZpin, HIGH);   // Outputs on
    
    b=b<<1;
    if (b>B00100000) {b=B00000001;}
  }
}

byte CheckButtonsState()
{
  static boolean buttonsWasChecked;
  static unsigned long startBuzzTime;
  static unsigned long lastTimeButtonsPressed;
  if ((digitalRead(pinSet)==0)||(digitalRead(pinUp)==0)||(digitalRead(pinDown)==0))
  {
    if (buttonsWasChecked==false) startBuzzTime=millis();
    buttonsWasChecked=true;
  } else buttonsWasChecked=false;
  if (millis()-startBuzzTime<30) 
    {
      digitalWrite(pinBuzzer, HIGH);
    } else
    {
      digitalWrite(pinBuzzer, LOW);
    }
}

String updateDisplayString()
{
  static  unsigned long lastTimeStringWasUpdated;
  if ((millis()-lastTimeStringWasUpdated)>1000)
  {
    lastTimeStringWasUpdated=millis();
    //RTC.getTime();
    //Serial.println(stringToDisplay);
    return PreZero(hour())+PreZero(minute())+PreZero(second());
  }
  return stringToDisplay;
}

void doTest()
{
  Serial.println("Start Test");
 byte b=B00000001;
 int curTube;
 String testStringArray[]={"000000","111111","222222","333333","444444","555555","666666","777777","888888","999999"};
 //String testStringArray[]={"123456","123456","123456","123456","123456","123456","123456","123456","123456","123456"};
 String testString;
 bool test=1;
 byte strIndex=0;
 unsigned long startOfTest=millis();
 while (test){
   
   if ((millis()-startOfTest)>500) 
   {
     startOfTest=millis();
     strIndex=strIndex+1;
     if (strIndex==10) test=0;
   }
   digitalWrite(HIZpin, LOW);    // High impedence outputs
   digitalWrite(LEpin, HIGH);    // allow data input (Transparent mode)
 
  curTube=int(log(b)/log(2));
 
  SPI.transfer(b|B11000000); // anode
  SPI.transfer(highBytesArray[testStringArray[strIndex].substring(curTube,curTube+1).toInt()]);
  SPI.transfer(lowBytesArray[testStringArray[strIndex].substring(curTube,curTube+1).toInt()]);
  
              //87654321 
  /*SPI.transfer(B11111111);
  SPI.transfer(B11111111);
  SPI.transfer(B11111110);*/
  

  digitalWrite(LEpin, LOW);     // latching data 
  //delay(1);
  digitalWrite(HIZpin, HIGH);   // Outputs on
  //while(1);
  b=b<<1;
  if (b>B00100000) {b=B00000001;}
  delayMicroseconds(2000);
  }; 
  Serial.println("Stop Test");
  
}

void doDotBlink()
{
  static unsigned long lastTimeBlink=millis();
  static bool dotState=0;
  if ((millis()-lastTimeBlink)>1000) 
  {
    lastTimeBlink=millis();
    dotState=!dotState;
    if (dotState) dotPattern=B11000000;
      else dotPattern=B00000000;
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

byte decToBcd(byte val){
// Convert normal decimal numbers to binary coded decimal
  return ( (val/10*16) + (val%10) );
}

byte bcdToDec(byte val)  {
// Convert binary coded decimal to normal decimal numbers
  return ( (val/16*10) + (val%16) );
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

byte doEditBlink()
{
  if (!BlinkUp) return B11111111;
  if (!BlinkDown) return B11111111;
  static unsigned long lastTimeEditBlink=millis();
  static bool blinkState=0;
  static byte tmp;
  if ((millis()-lastTimeEditBlink)>300) 
  {
    lastTimeEditBlink=millis();
    blinkState=!blinkState;
    if (blinkState) tmp= B11111111;
      else tmp=~blinkMask;
      //Serial.println()
  }
  return tmp;
}

int extractDigits(byte b)
{
  String tmp;
  if (b==B00000011) tmp=stringToDisplay.substring(0,2);
  if (b==B00001100) tmp=stringToDisplay.substring(2,4);
  if (b==B00110000) tmp=stringToDisplay.substring(4);
  return tmp.toInt();
}

void injectDigits(byte b, int value)
{
  if (b==B00000011) stringToDisplay=PreZero(value)+stringToDisplay.substring(2);
  if (b==B00001100) stringToDisplay=stringToDisplay.substring(0,2)+PreZero(value)+stringToDisplay.substring(4);
  if (b==B00110000) stringToDisplay=stringToDisplay.substring(0,4)+PreZero(value);
}

bool isValidDate()
{
  int days[12]={31,28,31,30,31,30,31,31,30,31,30,31};
  if (value[8]%4==0) days[1]=29;
  if (value[6]>days[value[7]-1]) return false;
    else return true;
  
}
