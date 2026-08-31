#ifndef __DOINDICATION_H
#define __DOINDICATION_H
//#include <stdint.h>
#include <Arduino.h>
unsigned int SymbolArray[10]={1, 2, 4, 8, 16, 32, 64, 128, 256, 512};
//unsigned int SymbolArray[10]={512, 256, 128, 64, 32, 16, 8, 4, 2, 1};
const unsigned int fpsLimit=16666;
const byte LEpin=10;
#define RHV5222PIN 8
bool HV5222;

#define NIXIE_GROUP_1_PIN 4
#define NIXIE_GROUP_2_PIN 5
#define NIXIE_GROUP_3_PIN 7

#define LOWER_DOTS_PIN 8
#define UPPER_DOTS_PIN 12

uint8_t ID1_data;

#define TubeOn 0x00
#define TubeOff 0x0F

void SPI_Init();
#endif
