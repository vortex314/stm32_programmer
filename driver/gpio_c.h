/*
 * gpio.h
 *
 *  Created on: Nov 17, 2015
 *      Author: lieven
 */

#ifndef INCLUDE_GPIO_C_H_
#define INCLUDE_GPIO_C_H_

void initPins();
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);
int analogRead(uint8_t pin);
void analogReference(uint8_t mode);
void analogWrite(uint8_t pin, int val);
void analogWriteFreq(uint32_t freq);
void analogWriteRange(uint32_t range);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);

typedef void (*voidFuncPtr)(void);
void attachInterrupt(uint8_t pin, voidFuncPtr userFunc, int mode);
void detachInterrupt(uint8_t pin) ;

static const uint8_t D0   = 16;
static const uint8_t D1   = 5;
static const uint8_t D2   = 4;
static const uint8_t D3   = 0;
static const uint8_t D4   = 2;
static const uint8_t D5   = 14;
static const uint8_t D6   = 12;
static const uint8_t D7   = 13;
static const uint8_t D8   = 15;
static const uint8_t D9   = 3;
static const uint8_t D10  = 1;

//Interrupt Modes
#define DISABLED  0x00
#define RISING    0x01
#define FALLING   0x02
#define CHANGE    0x03
#define ONLOW     0x04
#define ONHIGH    0x05
#define ONLOW_WE  0x0C
#define ONHIGH_WE 0x0D

#endif /* INCLUDE_GPIO_C_H_ */
