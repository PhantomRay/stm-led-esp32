#ifndef DISPLAY_LED_MATRIX_H
#define DISPLAY_LED_MATRIX_H

#include "command_parsing.h"

void display_init();
void display_task();
void set_queue(LED_COMMAND_QUEUE *cmd_queue);

#endif
