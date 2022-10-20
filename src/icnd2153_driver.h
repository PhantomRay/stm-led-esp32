#ifndef ICND2153_DRIVER_H__
#define ICND2153_DRIVER_H__

#include "Arduino.h"
#include "driver/gpio.h"

// leave this like it is, the pins are carefully selected to not interfere with special purpose pins
// also pins with the same function are grouped together so they can be set with one command to save CPU cycles
#define PIN_LE      GPIO_NUM_4  //4
#define PIN_LAT     PIN_LE
#define PIN_STROBE  PIN_LE
#define PIN_OE      GPIO_NUM_15 //15
#define PIN_CLK     GPIO_NUM_16 //16

#define PIN_A       GPIO_NUM_18 //18
#define PIN_B       GPIO_NUM_23 //23
#define PIN_C       GPIO_NUM_19 //19

#define PIN_R1      GPIO_NUM_27 //27
#define PIN_G1      GPIO_NUM_12 //12
#define PIN_B1      GPIO_NUM_14 //14
#define PIN_R2      GPIO_NUM_13 //13
#define PIN_G2      GPIO_NUM_26 //26
#define PIN_B2      GPIO_NUM_25 //25

#define PIN_MASK_CLK (1 << PIN_CLK)
#define PIN_MASK_LE  (1 << PIN_LE)
#define PIN_MASK_OE  (1 << PIN_OE)
#define PIN_MASK_A   (1 << PIN_A)
#define PIN_MASK_B   (1 << PIN_B) 
#define PIN_MASK_C   (1 << PIN_C) 
#define PIN_MASK_R1  (1 << PIN_R1) 
#define PIN_MASK_G1  (1 << PIN_G1)
#define PIN_MASK_B1  (1 << PIN_B1)
#define PIN_MASK_R2  (1 << PIN_R2)
#define PIN_MASK_G2  (1 << PIN_G2)
#define PIN_MASK_B2  (1 << PIN_B2)

#define PIN_MASK_SCAN   (PIN_MASK_A | PIN_MASK_B | PIN_MASK_C)
#define PIN_MASK_DATA   (PIN_MASK_R1 | PIN_MASK_G1 | PIN_MASK_B1 | PIN_MASK_R2 | PIN_MASK_G2 | PIN_MASK_B2)
#define PIN_MASK_ALL    (PIN_MASK_SCAN | PIN_MASK_DATA | PIN_MASK_LE | PIN_MASK_CLK | PIN_MASK_OE)

#define SET_LATCH     {GPIO.out_w1ts = PIN_MASK_LE;}
#define CLR_LATCH     {GPIO.out_w1tc = PIN_MASK_LE;}
#define SET_CLOCK     {GPIO.out_w1ts = PIN_MASK_CLK;}
#define CLR_CLOCK     {GPIO.out_w1tc = PIN_MASK_CLK;}
#define SET_OE        {GPIO.out_w1ts = PIN_MASK_OE;}
#define CLR_OE        {GPIO.out_w1tc = PIN_MASK_OE;}

void gpio_init();
void digitalWriteFast(uint8_t pinNum, bool value);
void digitalWriteEvenFaster(uint32_t data, uint32_t mask);
void IRAM_ATTR sendLatch(uint8_t clocks);
void InitICND2153(uint8_t chip_num);

#endif
