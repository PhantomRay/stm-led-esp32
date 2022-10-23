#ifndef COMMAND_H
#define COMMAND_H

#include "Arduino.h"

typedef struct{
    String type;
    String parameter;
}LED_COMMAND;

typedef struct{
    LED_COMMAND cmd;
    void* qe_next;
}LED_COMMAND_DESCRIPTION;

extern LED_COMMAND_DESCRIPTION *command_desc_first;
extern bool update_display_flag;

void command_init();
void command_task(void *pvParameter);

#endif
