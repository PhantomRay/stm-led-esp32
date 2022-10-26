#ifndef DISPLAY_LED_MATRIX_H
#define DISPLAY_LED_MATRIX_H

#include "command_parsing.h"

extern LED_COMMAND_DESCRIPTION *display_command;

void display_init();
void update_display_param(LED_COMMAND_DESCRIPTION *cmd_desc_first);
// void display_task(void *pvParameter);

#endif
