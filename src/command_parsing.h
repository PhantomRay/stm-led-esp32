#ifndef COMMAND_H
#define COMMAND_H

#include "Arduino.h"

typedef struct {
  String type;
  String parameter;
} LED_COMMAND;

typedef struct {
  LED_COMMAND cmd;
  void *qe_next;
} LED_COMMAND_DESCRIPTION;

extern LED_COMMAND_DESCRIPTION *command_desc[2];
extern uint8_t current_display_description_id;
extern volatile bool command_desc_update_flag, command_desc_stop_flag;

void command_init();
void command_task(void *pvParameter);

#endif
