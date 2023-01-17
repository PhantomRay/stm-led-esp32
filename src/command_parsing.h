#ifndef COMMAND_H
#define COMMAND_H

#include "Arduino.h"

// #define COMMAND_DEBUG
// #define COMMAND_SERIAL
#ifdef COMMAND_SERIAL
#define SerialCommand Serial1
#define COMMAND_RX_PIN GPIO_NUM_21
#define COMMAND_TX_PIN GPIO_NUM_22
#else
#define SerialCommand Serial
#endif

typedef struct {
  String type;
  String parameter;
} LED_COMMAND;

typedef struct {
  LED_COMMAND cmd;
  void *qe_next;
} LED_COMMAND_DESCRIPTION;

typedef struct {
  LED_COMMAND_DESCRIPTION *first;
  LED_COMMAND_DESCRIPTION *last;
  uint16_t count;
  bool stop_flag;
} LED_COMMAND_QUEUE;

typedef struct {
  int8_t r;
  int8_t g;
  int8_t b;
} COLOR_DESC;

typedef struct {
  int16_t x;
  int16_t y;
} POSITION_DESC;

typedef struct {
  POSITION_DESC pos;
  COLOR_DESC color;
} Pixel_DESC;

typedef struct {
  POSITION_DESC pos;
  int16_t radius;
  COLOR_DESC color;
} CIRCLE_DESC;

typedef struct {
  POSITION_DESC pos;
  int16_t width;
  int16_t height;
  COLOR_DESC color;
} RECT_DESC;

typedef struct {
  uint16_t times;
  uint32_t delay;
} FLASHER_DESC;

void command_init();
void show_loading();
void command_task(void *pvParameter);
void clear_command_desc(LED_COMMAND_DESCRIPTION *p_command_desc_first);

void print_hex(String &buf);

#endif
