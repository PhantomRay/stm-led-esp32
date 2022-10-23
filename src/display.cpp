#include "display.h"
#include "ESP32-ICND2153-I2S-DMA.h"
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeSerif12pt7b.h>

void display_init() { led_matrix.begin(); }
bool get_integer(String color_str, uint8_t color[], uint8_t num) {
  bool ret_val  = true;
  int start_pos = 0;
  int end_pos;
  int str_len = color_str.length();
  int id      = 0;
  while (start_pos < str_len) {
    end_pos = color_str.indexOf(',', start_pos);
    if (end_pos == -1) {
      end_pos = str_len;
    }
    if (id < num) {
      color[id] = color_str.substring(start_pos, end_pos).toInt();
      id++;
    } else {
      ret_val = false;
      break;
    }
    start_pos = end_pos + 1;
  }
  return ret_val;
}
void update_display_param(LED_COMMAND_DESCRIPTION *cmd_desc_first) {
  LED_COMMAND_DESCRIPTION *command_desc_tmp = cmd_desc_first;
  String cmd_type;
  String cmd_parm;
  while (command_desc_tmp != NULL) {
    cmd_type = command_desc_tmp->cmd.type;
    cmd_parm = command_desc_tmp->cmd.parameter;

    if (cmd_type == "CL") {
      led_matrix.fillScreenRGB888(0, 0, 0);
    } else if (cmd_type == "BR") {
      uint8_t bright = cmd_parm.toInt();
      if (bright < 100) {
        led_matrix.setPanelBrightness(bright);
      } else {
        Serial.println("Brightness is less than 100.");
      }
    } else if (cmd_type == "F") {
      if (cmd_parm == "default") {
        led_matrix.setFont();
      } else if (cmd_parm == "FreeMono9pt7b") {
        led_matrix.setFont(&FreeMono9pt7b);
      } else if (cmd_parm == "FreeMono12pt7b") {
        led_matrix.setFont(&FreeMono9pt7b);
      } else if (cmd_parm == "FreeSans9pt7b") {
        led_matrix.setFont(&FreeSans9pt7b);
      } else if (cmd_parm == "FreeSans12pt7b") {
        led_matrix.setFont(&FreeSans12pt7b);
      } else if (cmd_parm == "FreeSerif9pt7b") {
        led_matrix.setFont(&FreeSans9pt7b);
      } else if (cmd_parm == "FreeSerif12pt7b") {
        led_matrix.setFont(&FreeSans12pt7b);
      }
    } else if (cmd_type == "S") {
      // Desired text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
      led_matrix.setTextSize(cmd_parm.toInt());
      // Serial.println("Font Size no support. Please use with Font parameter");
    } else if (cmd_type == "BG") {
      uint8_t rgb[3];
      if (get_integer(cmd_parm, rgb, 3)) {
        led_matrix.fillScreenRGB888(rgb[0], rgb[1], rgb[2]);
      } else {
        Serial.println("Err:Background Color");
      }
    } else if (cmd_type == "TC") {
      uint8_t rgb[3];
      if (get_integer(cmd_parm, rgb, 3)) {
        led_matrix.setTextColor(led_matrix.color565(rgb[0], rgb[1], rgb[2]));
      } else {
        Serial.println("Err:Text Color");
      }
    } else if (cmd_type == "CR") {
      uint8_t pos_xy[2];
      if (get_integer(cmd_parm, pos_xy, 2)) {
        led_matrix.setCursor(pos_xy[0], pos_xy[1]);
      } else {
        Serial.println("Err:Text Color");
      }
    } else if (cmd_type == "P") {
      uint8_t pos_xy[2];
      led_matrix.println(cmd_parm);
    } else if (cmd_type == "D") {
      led_matrix.showDMABuffer(true);
      delay(cmd_parm.toInt());
    }

    command_desc_tmp = (LED_COMMAND_DESCRIPTION *)(command_desc_tmp->qe_next);
  }
}
