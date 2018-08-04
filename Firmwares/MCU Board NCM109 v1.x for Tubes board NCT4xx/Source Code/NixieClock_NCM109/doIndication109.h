#ifndef __DOINDICATION_H
#define __DOINDICATION_H
//#include <stdint.h>
#include <Arduino.h>
unsigned int SymbolArray[10]={1, 2, 4, 8, 16, 32, 64, 128, 256, 512};
const unsigned int fpsLimit=16666;
const byte LEpin=10;

#define AnodeGroup1ON 0x100000
#define AnodeGroup2ON 0x200000
#define AnodeGroup3ON 0x400000

#define TubeOn 0xFFFFFFFF
#define TubeOff 0xFC00
#endif
