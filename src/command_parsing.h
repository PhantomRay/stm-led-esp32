#ifndef COMMAND_H
#define COMMAND_H

#include "Arduino.h"

// #define COMMAND_DEBUG
#define SerialCommand Serial // Serial1
#define COMMAND_RX_PIN GPIO_NUM_21
#define COMMAND_TX_PIN GPIO_NUM_22

typedef struct {
  String type;
  String parameter;
} LED_COMMAND;

typedef struct {
  LED_COMMAND cmd;
  void *qe_next;
} LED_COMMAND_DESCRIPTION;

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
  uint16_t delay;
} FLASHER_DESC;

extern LED_COMMAND_DESCRIPTION *command_desc[2];
extern uint8_t current_display_description_id;
extern volatile bool command_desc_update_flag, command_desc_stop_flag;

void command_init();
void command_task(void *pvParameter);

#endif
