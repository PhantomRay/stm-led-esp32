#include "Arduino.h"
#include "command_parsing.h"
#include "command_animation.h"
#include "display.h"

// CL;F:font1;S:10;BG:255,0,0;TC:0,255,0;CR:10,20;P:\\HELLO, WORLD!\\;D:5000;CL\r
// This means, clear the screen, set font to "font1", text size 10, set background to 255,0,0, text color 0,255,0,
// cursor set to 10,20, print "HELLO, WORLD!"

// command_type_num = sizeof(command_type) / sizeof(command_type[0]);
#define COMMAND_TYPE_NUM 16
const String command_type[COMMAND_TYPE_NUM] = {"CL", "BR", "FT", "SZ", "BG", "TC", "CR", "PT",
                                               "IM", "CI", "RT", "DL", "FL", "AN", "FS", "HP"};
String command_buffer = ""; // ="CL;F:font1;S:10;BG:255,0,0;TC:0,255,0;CR:10,20;P:\\HELLO, WORLD!\\;D:5000;CL\r";
const char command_separate_char   = ';';
const char parameter_separate_char = ':';

LED_COMMAND_DESCRIPTION *command_desc_first = NULL;
LED_COMMAND_DESCRIPTION *command_desc_last  = NULL;

bool command_desc_dynamic_alloc_property[2];
LED_COMMAND_DESCRIPTION *command_desc[2];
uint8_t current_display_description_id = 0;

volatile bool command_desc_update_flag = false;
volatile bool command_desc_stop_flag   = false; // for FL command
bool command_desc_dynamic_alloc_flag   = true;

static void clear_command_desc(LED_COMMAND_DESCRIPTION *p_command_desc_first) {
  int command_num = 0;
  LED_COMMAND_DESCRIPTION *command_desc_next;
  LED_COMMAND_DESCRIPTION *command_desc_tmp = p_command_desc_first;
  while (command_desc_tmp != NULL) {
    command_desc_next = (LED_COMMAND_DESCRIPTION *)(command_desc_tmp->qe_next);
    delete command_desc_tmp;
    command_desc_tmp = command_desc_next;
    command_num++;
  }
  SerialCommand.printf("%d commands is cleared.\n", command_num);
}

static void command_parsing(const String cmd_buf) {
  int start_pos = 0;
  int end_pos   = 0;
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
    // SerialCommand.printf("%s\t", cmd_string);
    // for(int char_id = 0; char_id < cmd_string.length(); char_id++){
    //     SerialCommand.printf("%x,", cmd_string.c_str()[char_id]);
    // }
    // SerialCommand.println();
  }

  int cmd_num                               = 0;
  LED_COMMAND_DESCRIPTION *command_desc_tmp = command_desc_first;
  while (command_desc_tmp != NULL) {
    command_desc_tmp = (LED_COMMAND_DESCRIPTION *)(command_desc_tmp->qe_next);
    cmd_num++;
  }
  SerialCommand.printf("Command:%d\n", cmd_num);

  /*animation command*/
  command_desc_dynamic_alloc_flag = true;
  if (command_desc_first != NULL) {
    if (command_desc_first->cmd.type == "AN") {
      clear_command_desc(command_desc_first);
      command_desc_first              = get_animation_desc();
      command_desc_dynamic_alloc_flag = false;
    }
  }
}

void command_init() {
  // SerialCommand.begin(115200, SERIAL_8N1, COMMAND_RX_PIN, COMMAND_TX_PIN);
  delay(100);
  command_buffer = "";

  command_desc[0]                        = NULL;
  command_desc[1]                        = NULL;
  command_desc_dynamic_alloc_property[0] = true;
  command_desc_dynamic_alloc_property[1] = true;
  current_display_description_id         = 0;

  command_desc_update_flag        = false;
  command_desc_stop_flag          = false;
  command_desc_dynamic_alloc_flag = true;

  command_animation_init();
  SerialCommand.setTimeout(200);
}

static void update_command_desc(LED_COMMAND_DESCRIPTION *new_desc, bool dynamic_flag) {
  if (current_display_description_id == 0) {
    current_display_description_id = 1;
  } else {
    current_display_description_id = 0;
  }
  command_desc[current_display_description_id]                        = new_desc;
  command_desc_dynamic_alloc_property[current_display_description_id] = dynamic_flag;

  command_desc_update_flag = true;
  SerialCommand.print("New command list is requested/");
  while (command_desc_update_flag) {
    vTaskDelay(200 / portTICK_PERIOD_MS);
  };
  SerialCommand.println("accepted");

  command_desc_stop_flag = false;
  uint8_t last_desc_id   = 0;
  if (current_display_description_id == 0) {
    last_desc_id = 1;
  } else {
    last_desc_id = 0;
  }

  if (command_desc_dynamic_alloc_property[last_desc_id]) {
    clear_command_desc(command_desc[last_desc_id]);
    command_desc[last_desc_id] = NULL;
  }
}

#define MAX_COMMAND_STRING 256
void command_task(void *pvParameter) {
  String rx_buf;
  while (1) {
    SerialCommand.println("Input a new Command:");
    while (1) {
      if (SerialCommand.available() > 0) {
        rx_buf = SerialCommand.readStringUntil('\r'); // SerialCommand.readString();
        command_buffer += rx_buf;
        SerialCommand.printf("%s", rx_buf);
        if (command_buffer.length() > MAX_COMMAND_STRING) {
          SerialCommand.println("Command String is longer than 256bytes.Please input again.\n");
          command_buffer = "";
        } else {
          if ((rx_buf.indexOf("\r") != -1) || (rx_buf.indexOf("\n") != -1)) {
            // SerialCommand.printf("command string: %s", command_buffer);
            break;
          }
        }
      }
      vTaskDelay(500 / portTICK_PERIOD_MS);
      // SerialCommand.println("wait command");
    }

    // SerialCommand.println("command parsing\n");
    command_parsing(command_buffer);

    update_command_desc(command_desc_first, command_desc_dynamic_alloc_flag);
    command_desc_first = NULL;
    command_buffer     = "";
    // SerialCommand.flush();
  }
}
