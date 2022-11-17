#include "Arduino.h"
#include "command_parsing.h"
#include "display.h"

// CL;F:font1;S:10;BG:255,0,0;TC:0,255,0;CR:10,20;P:\\HELLO, WORLD!\\;D:5000;CL\r
// This means, clear the screen, set font to "font1", text size 10, set background to 255,0,0, text color 0,255,0,
// cursor set to 10,20, print "HELLO, WORLD!"

// command_type_num = sizeof(command_type) / sizeof(command_type[0]);
#define COMMAND_TYPE_NUM 15
const String command_type[COMMAND_TYPE_NUM] = {"CL", "BR", "FT", "SZ", "BG", "TC", "CR", "PT",
                                               "IM", "CI", "RT", "DL", "FL", "FS", "HP"};
String command_buffer = ""; // ="CL;F:font1;S:10;BG:255,0,0;TC:0,255,0;CR:10,20;P:\\HELLO, WORLD!\\;D:5000;CL\r";
const char command_separate_char   = ';';
const char parameter_separate_char = ':';

LED_COMMAND_QUEUE new_queue;

void clear_command_desc(LED_COMMAND_DESCRIPTION *p_command_desc_first) {
  int command_num = 0;
  LED_COMMAND_DESCRIPTION *command_desc_next;
  LED_COMMAND_DESCRIPTION *command_desc_tmp = p_command_desc_first;
  while (command_desc_tmp != NULL) {
    command_desc_next = (LED_COMMAND_DESCRIPTION *)(command_desc_tmp->qe_next);
    delete command_desc_tmp;
    command_desc_tmp = command_desc_next;
    command_num++;
  }
  Serial.printf("%d commands is cleared.\n", command_num);
}

static LED_COMMAND_QUEUE *command_parsing(const String cmd_buf) {
  LED_COMMAND_DESCRIPTION *command_desc_first = NULL;
  LED_COMMAND_DESCRIPTION *command_desc_last  = NULL;
  bool command_desc_stop_flag                 = false;
  int start_pos                               = 0;
  int end_pos                                 = 0;
  String cmd_string;
  int commands_string_length = cmd_buf.length();

  while (start_pos < commands_string_length) {
    end_pos = cmd_buf.indexOf(command_separate_char, start_pos);
    if (end_pos == -1) {
      end_pos = commands_string_length;
    }
    cmd_string = cmd_buf.substring(start_pos, end_pos);
    start_pos  = end_pos + 1;

    int cmd_type_pos, cmd_parmeter_pos;
    LED_COMMAND_DESCRIPTION *cmd_desc;
    for (int i = 0; i < COMMAND_TYPE_NUM; i++) {
      cmd_type_pos = cmd_string.indexOf(command_type[i]);
      if (cmd_type_pos == 0) {
        if (command_type[i] == "FL") {
          command_desc_stop_flag = true;
        } else {
          cmd_desc           = new LED_COMMAND_DESCRIPTION;
          cmd_desc->cmd.type = command_type[i];
          if (command_type[i] == "CL") {
            cmd_desc->cmd.parameter = "";
          } else {
            cmd_parmeter_pos = cmd_string.indexOf(parameter_separate_char);
            if (cmd_parmeter_pos != -1) {
              cmd_desc->cmd.parameter = cmd_string.substring(cmd_parmeter_pos + 1, cmd_string.length());
            }
          }
          cmd_desc->qe_next = NULL;
          if (command_desc_first == NULL) {
            command_desc_first = cmd_desc;
            command_desc_last  = command_desc_first;
          } else {
            command_desc_last->qe_next = cmd_desc;
            command_desc_last          = cmd_desc;
          }
        }
        break;
      }
    }
  }

  int cmd_num                               = 0;
  LED_COMMAND_DESCRIPTION *command_desc_tmp = command_desc_first;
  while (command_desc_tmp != NULL) {
    command_desc_tmp = (LED_COMMAND_DESCRIPTION *)(command_desc_tmp->qe_next);
    cmd_num++;
  }
  Serial.printf("Command:%d\n", cmd_num);

  new_queue.first     = command_desc_first;
  new_queue.last      = command_desc_last;
  new_queue.count     = cmd_num;
  new_queue.stop_flag = command_desc_stop_flag;
  return &new_queue;
}

void command_init() {
  // SerialCommand.begin(115200, SERIAL_8N1, COMMAND_RX_PIN, COMMAND_TX_PIN);
  delay(100);
  command_buffer = "";

  SerialCommand.setTimeout(200);
}

#define MAX_COMMAND_STRING 256
void command_task(void *pvParameter) {
  String rx_buf;
  while (1) {
    Serial.println("Input a new Command:");
    while (1) {
      if (SerialCommand.available() > 0) {
        rx_buf = SerialCommand.readStringUntil('\r');
        command_buffer += rx_buf;
        Serial.printf("%s", rx_buf);
        if (command_buffer.length() > MAX_COMMAND_STRING) {
          Serial.println("Command String is longer than 256bytes.Please input again.\n");
          command_buffer = "";
        } else {
          if ((rx_buf.indexOf("\r") != -1) || (rx_buf.indexOf("\n") != -1)) {
            break;
          }
        }
      }
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    LED_COMMAND_QUEUE *cmd_queue = command_parsing(command_buffer);
    set_queue(cmd_queue);
    command_buffer = "";
  }
}
