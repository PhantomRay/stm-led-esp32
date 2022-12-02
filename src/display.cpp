#include "SPIFFS.h"
#include "display.h"
#include "ESP32-ICND2153-I2S-DMA.h"

// https://rop.nl/truetype2gfx/

#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSans46pt7b.h>

#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold46pt7b.h>

typedef struct {
  String name;
  const GFXfont *param;
} FONT_INFO;
#define FONT_INFO_NUM 10
FONT_INFO font_inf[FONT_INFO_NUM] = {
    {"FreeSans9pt7b", &FreeSans9pt7b},           {"FreeSans12pt7b", &FreeSans12pt7b},
    {"FreeSans18pt7b", &FreeSans18pt7b},         {"FreeSans24pt7b", &FreeSans24pt7b},
    {"FreeSans46pt7b", &FreeSans46pt7b},         {"FreeSansBold9pt7b", &FreeSansBold9pt7b},
    {"FreeSansBold12pt7b", &FreeSansBold12pt7b}, {"FreeSansBold18pt7b", &FreeSansBold18pt7b},
    {"FreeSansBold24pt7b", &FreeSansBold24pt7b}, {"FreeSansBold46pt7b", &FreeSansBold46pt7b},
};
FONT_INFO *current_font_inf = NULL;

LED_COMMAND_QUEUE command_queue = {NULL, NULL, 0, false};
bool command_queue_stop_flag    = false;

void display_init() {
  command_queue           = {NULL, NULL, 0, false};
  command_queue_stop_flag = false;

  led_matrix.begin();
  command_init();
}

bool parse_string(String in_str, String out_str[], size_t str_num) {
  bool ret_val  = true;
  int start_pos = 0;
  int end_pos;
  int str_len = in_str.length();
  int id      = 0;
  while (start_pos < str_len) {
    end_pos = in_str.indexOf(',', start_pos);
    if (end_pos == -1) {
      end_pos = str_len;
    }
    if (id < str_num) {
      out_str[id] = in_str.substring(start_pos, end_pos);
      id++;
    } else {
      // ret_val = false;
      break;
    }
    start_pos = end_pos + 1;
  }

  if (id < str_num) {
    ret_val = false;
  }
#ifdef COMMAND_DEBUG
  else {
    Serial.printf("Parsing Ok:");
    for (int i = 0; i < str_num; i++) {
      Serial.printf("%s/", out_str[i]);
    }
    Serial.println("");
  }
#endif
  return ret_val;
}

static bool get_position(String parm_str, POSITION_DESC *param_position) {
  bool ret_val = false;
  String parm_string_array[2];
  if (parse_string(parm_str, parm_string_array, 2)) {
    param_position->x = parm_string_array[0].toInt();
    param_position->y = parm_string_array[1].toInt();
    ret_val           = true;
  }
  return ret_val;
}

static bool get_color(String parm_str, COLOR_DESC *param_color) {
  bool ret_val = false;
  String parm_string_array[3];
  if (parse_string(parm_str, parm_string_array, 3)) {
    param_color->r = parm_string_array[0].toInt();
    param_color->g = parm_string_array[1].toInt();
    param_color->b = parm_string_array[2].toInt();
    ret_val        = true;
  }
  return ret_val;
}

static bool get_circle_par(String parm_str, CIRCLE_DESC *parm_circle) {
  bool ret_val = false;
  String parm_string_array[6];
  if (parse_string(parm_str, parm_string_array, 6)) {
    parm_circle->pos.x   = parm_string_array[0].toInt();
    parm_circle->pos.y   = parm_string_array[1].toInt();
    parm_circle->radius  = parm_string_array[2].toInt();
    parm_circle->color.r = parm_string_array[3].toInt();
    parm_circle->color.g = parm_string_array[4].toInt();
    parm_circle->color.b = parm_string_array[5].toInt();
    ret_val              = true;
  }
  return ret_val;
}

static bool get_rectangle_par(String parm_str, RECT_DESC *parm_rectangle) {
  bool ret_val = false;
  String parm_string_array[7];
  if (parse_string(parm_str, parm_string_array, 7)) {
    parm_rectangle->pos.x   = parm_string_array[0].toInt();
    parm_rectangle->pos.y   = parm_string_array[1].toInt();
    parm_rectangle->width   = parm_string_array[2].toInt();
    parm_rectangle->height  = parm_string_array[3].toInt();
    parm_rectangle->color.r = parm_string_array[4].toInt();
    parm_rectangle->color.g = parm_string_array[5].toInt();
    parm_rectangle->color.b = parm_string_array[6].toInt();
    ret_val                 = true;
  }
  return ret_val;
}

static bool get_fs_par(String parm_str, FLASHER_DESC *parm_fs) {
  bool ret_val = false;
  String parm_string_array[2];
  if (parse_string(parm_str, parm_string_array, 2)) {
    parm_fs->times = (uint16_t)parm_string_array[0].toInt();
    parm_fs->delay = (uint32_t)parm_string_array[1].toInt();
    ret_val        = true;
  }
  return ret_val;
}

static uint16_t read16(File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

static uint32_t read32(File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

static void drawBmpFromFile(String filename, uint8_t xMove, uint16_t yMove) {
  //   Serial.println("In drawBmpFromFile");
  File bmpFile;
  int bmpWidth, bmpHeight;            // W+H in pixels
  uint8_t bmpDepth;                   // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;            // Start of image data in file
  uint32_t rowSize;                   // Not always = bmpWidth; may have padding
  uint8_t sdbuffer[3 * 20];           // pixel buffer (R+G+B per pixel)
  uint8_t buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean goodBmp = false;            // Set to true on valid header parse
  boolean flip    = true;             // BMP is stored bottom-to-top
  int w, h, row, col;
  uint8_t r, g, b;
  uint32_t pos = 0, startTime = millis();

  if ((xMove >= led_matrix.width()) || (yMove >= led_matrix.height())) return;

  /*Serial.println();
  Serial.print(F("Loading image '"));
  Serial.print(filename);
  Serial.println('\'');*/

  bmpFile = SPIFFS.open("/" + filename, "r");
  // Open requested file on SD card
  if (!bmpFile) {
    Serial.print(F("File not found"));
    return;
  }

  // Parse BMP header
  if (read16(bmpFile) == 0x4D42) { // BMP signature
    // Serial.print(F("File size: "));
    uint32_t filesize = read32(bmpFile);
    // Serial.println(filesize);
    (void)read32(bmpFile);            // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    // Serial.print(F("Image Offset: ")); Serial.println(bmpImageoffset, DEC);
    //  Read DIB header
    // Serial.print(F("Header size: "));
    uint32_t headerSize = read32(bmpFile);
    bmpWidth            = read32(bmpFile);
    bmpHeight           = read32(bmpFile);
    if (read16(bmpFile) == 1) {     // # planes -- must be '1'
      bmpDepth = read16(bmpFile);   // bits per pixel
                                    //   Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
      if ((read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        /*Serial.print(F("Image size: "));
        Serial.print(bmpWidth);
        Serial.print('x');
        Serial.println(bmpHeight);*/

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if (bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if ((xMove + w - 1) >= led_matrix.width()) w = led_matrix.width() - xMove;
        if ((yMove + h - 1) >= led_matrix.height()) h = led_matrix.height() - yMove;

        if ((bmpDepth == 24)) {
          for (row = 0; row < h; row++) { // For each scanline...

            // Seek to start of scan line.  It might seem labor-
            // intensive to be doing this on every line, but this
            // method covers a lot of gritty details like cropping
            // and scanline padding.  Also, the seek only takes
            // place if the file position actually needs to change
            // (avoids a lot of cluster math in SD library).
            if (flip) // Bitmap is stored bottom-to-top order (normal BMP)
              pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
            else // Bitmap is stored top-to-bottom
              pos = bmpImageoffset + row * rowSize;
            if (bmpFile.position() != pos) { // Need seek?
              bmpFile.seek(pos, SeekSet);
              buffidx = sizeof(sdbuffer); // Force buffer reload
            }

            for (col = 0; col < w; col++) { // For each pixel...
              // Time to read more pixel data?
              if (buffidx >= sizeof(sdbuffer)) { // Indeed
                bmpFile.read(sdbuffer, sizeof(sdbuffer));
                buffidx = 0; // Set index to beginning
              }

              // Convert pixel from BMP to TFT format, push to display
              b = sdbuffer[buffidx++];
              g = sdbuffer[buffidx++];
              r = sdbuffer[buffidx++];
              led_matrix.drawPixelRGB888(col + xMove, row + yMove, r, g, b);

            } // end pixel
          }   // end scanline
        } else if (bmpDepth == 1) {
        }

      } // end goodBmp
    }
  }

  bmpFile.close();
  if (!goodBmp) Serial.println(F("BMP format not recognized."));
}

bool isStopCondition() {
  if (command_queue_stop_flag == true) {
    return true;
  }
  return false;
}

bool display_delay(uint32_t timeout_ms) {
  led_matrix.showDMABuffer(true);
  bool ret_val = false;
  while (timeout_ms >= 50) {
    if (isStopCondition()) {
      ret_val = true;
      break;
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
    timeout_ms -= 50;
  };
  return ret_val;
}

void set_queue(LED_COMMAND_QUEUE *cmd_queue) {
  if ((cmd_queue->stop_flag) || (command_queue.first == NULL)) {
    command_queue.first     = cmd_queue->first;
    command_queue.last      = cmd_queue->last;
    command_queue.count     = cmd_queue->count;
    command_queue_stop_flag = true;
    while (command_queue_stop_flag) {
      vTaskDelay(50 / portTICK_PERIOD_MS);
    };

#ifdef COMMAND_DEBUG
    Serial.print("New command queue start");
#endif
  } else {
    if (cmd_queue->first != NULL) {
      command_queue.last->qe_next = cmd_queue->first;
      command_queue.last          = cmd_queue->last;
      command_queue.count += cmd_queue->count;
#ifdef COMMAND_DEBUG
      Serial.printf("Add %d commands\n", cmd_queue->count);
#endif
    }
  }
  vTaskDelay(50 / portTICK_PERIOD_MS);
}

void get_text_newline(String &analy_string, String &out_string, int16_t *ul_x, int16_t *ul_y, uint16_t *w,
                      uint16_t *h) {
  int16_t x, y;
  int pos;
  pos = analy_string.indexOf("\\");
  if (pos == -1) {
    out_string   = analy_string;
    analy_string = "";
  } else {
    out_string   = analy_string.substring(0, pos);
    analy_string = analy_string.substring(pos + 1);
  }
#ifdef COMMAND_DEBUG
  Serial.printf("New Line:%s\t", out_string);
// for (int i = 0; i < new_str.length(); i++) {
//   Serial.printf("%x ", new_str.c_str()[i]);
// }
#endif

  if (current_font_inf == NULL) {
    x = 0;
    y = 0;
  } else {
    x = 0;
    y = led_matrix.height() - 1;
  }
  led_matrix.getTextBounds(out_string, x, y, ul_x, ul_y, w, h);
  // SerialCommand.printf("x=%d,y=%d,x1=%d,y1=%d,w=%d,h=%d\n", x, y, *ul_x, *ul_y, *w, *h);
}

bool get_textalignleft_cursor(uint16_t w, uint16_t h, int16_t *new_x, int16_t *new_y) {
  int16_t x = led_matrix.getCursorX();
  int16_t y = led_matrix.getCursorY();
  // SerialCommand.printf("cur_x=%d,cur_y=%d\n", x, y);
  if (current_font_inf == NULL) {
    if ((y + 6) > led_matrix.height()) {
      y = 0;
    }
  } else {
    if ((y >= led_matrix.height()) || ((y + 1) < h)) {
      y = h - 1;
    }
  }

  *new_x = 0;
  *new_y = y;

  return true;
}

bool get_textaligncenter_cursor(int16_t ulx, int16_t uly, uint16_t w, uint16_t h, int16_t *new_x, int16_t *new_y) {
  int16_t x = led_matrix.getCursorX();
  int16_t y = led_matrix.getCursorY();
  // SerialCommand.printf("cur_x=%d,cur_y=%d\n", x, y);
  if (current_font_inf == NULL) {
    if ((y + 6) > led_matrix.height()) {
      y = 0;
    }
  } else {
    if ((y >= led_matrix.height()) || ((y + 1) < h)) {
      y = h - 1;
    }
  }
  x = (led_matrix.width() - w) / 2;
  if (x >= ulx) {
    x -= ulx;
  } else {
    x = 0;
  }
  // SerialCommand.printf("new_x=%d,new_y=%d\n", x, y);
  *new_x = x;
  *new_y = y;

  return true;
}

void check_cursor() {
  int16_t x = led_matrix.getCursorX();
  int16_t y = led_matrix.getCursorY();
  if ((x < 0) || (x >= led_matrix.width())) {
    x = 0;
  }
  if ((y < 0) || (y >= led_matrix.height())) {
    y = 0;
  }

  led_matrix.setCursor(x, y);
}

void display_task() {
  LED_COMMAND_DESCRIPTION *command_desc_curr;
  while (true) {
    if (command_queue_stop_flag) {
      if (command_queue.first != NULL) {
        command_desc_curr       = command_queue.first;
        command_queue_stop_flag = false;
        break;
      }
      command_queue_stop_flag = false;
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
#ifdef COMMAND_DEBUG
  Serial.printf("New commands %d\n", command_queue.count);
#endif
  String cmd_type;
  String cmd_parm;
  while (true) {
    cmd_type = command_desc_curr->cmd.type;
    cmd_parm = command_desc_curr->cmd.parameter;
#ifdef COMMAND_DEBUG
    Serial.printf("run_type:%s, run_param:%s\n", cmd_type, cmd_parm);
#endif

    if (cmd_type == "CL") {
      led_matrix.fillScreenRGB888(0, 0, 0);
      led_matrix.setCursor(0, 0);
    } else if (cmd_type == "BR") {
      uint8_t bright = cmd_parm.toInt();
      if (bright < 100) {
        led_matrix.setPanelBrightness(bright);
      } else {
        Serial.println("Brightness is less than 100.");
      }
    } else if (cmd_type == "FT") {
      if ((cmd_parm == "default") || (cmd_parm == "")) {
        led_matrix.setFont();
        current_font_inf = NULL;
      } else {
        led_matrix.setTextSize(1);
        int i;
        for (i = 0; i < FONT_INFO_NUM; i++) {
          if (cmd_parm == font_inf[i].name) {
            led_matrix.setFont(font_inf[i].param);
            current_font_inf = &font_inf[i];
            break;
          }
        }
        if (i == FONT_INFO_NUM) {
          Serial.println("Font no support.");
        }
      }
      check_cursor();
    } else if (cmd_type == "SZ") {
      // use only for defualt font
      //  Desired text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
      led_matrix.setTextSize(cmd_parm.toInt());
      // Serial.println("Font Size no support. Please use with Font parameter");
    } else if (cmd_type == "BG") {
      COLOR_DESC rgb;
      if (get_color(cmd_parm, &rgb)) {
        led_matrix.fillScreenRGB888(rgb.r, rgb.g, rgb.b);
      } else {
        Serial.println("Err:Background Color");
      }
    } else if (cmd_type == "TC") {
      COLOR_DESC rgb;
      if (get_color(cmd_parm, &rgb)) {
        led_matrix.setTextColor(led_matrix.color565(rgb.r, rgb.g, rgb.b));
      } else {
        Serial.println("Err:Text Color");
      }
    } else if (cmd_type == "CR") {
      POSITION_DESC pos_xy;
      if (get_position(cmd_parm, &pos_xy)) {
        led_matrix.setCursor(pos_xy.x, pos_xy.y);
      } else {
        Serial.println("Err:Text Color");
      }
    } else if (cmd_type == "GR") { // grid
      uint16_t color16 = led_matrix.color565(255, 0, 0);
      for (int i = 7; i < led_matrix.width(); i += 8) {
        led_matrix.drawLine(i, 0, i, led_matrix.height() - 1, color16);
      }
      for (int i = 7; i < led_matrix.height(); i += 8) {
        led_matrix.drawLine(0, i, led_matrix.width() - 1, i, color16);
      }
    } else if (cmd_type == "PT") { // print a string
      if (current_font_inf == NULL) {
        check_cursor();
        String new_string = cmd_parm;
        new_string.replace("\\", "\n");
        print_hex(new_string);
        led_matrix.println(new_string);
      } else {
        int16_t x, y, x1, y1;
        uint16_t w, h;
        String new_str = "";

        while (1) {
          if (cmd_parm == "") {
            break;
          }
          get_text_newline(cmd_parm, new_str, &x1, &y1, &w, &h);
          if (get_textalignleft_cursor(w, h, &x, &y)) {
            led_matrix.setCursor(x, y);
            led_matrix.println(new_str);
          }
        }
      }
    } else if (cmd_type == "PC") { // print a string with hozontal center alignment
      int16_t x, y, x1, y1;
      uint16_t w, h;
      String new_str = "";

      while (1) {
        if (cmd_parm == "") {
          break;
        }
        get_text_newline(cmd_parm, new_str, &x1, &y1, &w, &h);
        if (get_textaligncenter_cursor(x1, y1, w, h, &x, &y)) {
          led_matrix.setCursor(x, y);
          led_matrix.println(new_str);
        }
      }
    } else if (cmd_type == "SC") { // print a string with screem center alignment
      int16_t x, y, x1, y1;
      uint16_t w, h;
      String new_str        = "";
      String tmp_str        = cmd_parm;
      uint16_t total_height = 0;
      uint16_t last_space   = 0;
      uint16_t line_height  = 8; // default font New line distance
      if (current_font_inf != NULL) {
        line_height = current_font_inf->param->yAdvance;
      }

      // estimate total height
      get_text_newline(cmd_parm, new_str, &x1, &y1, &w, &h);
      if (h <= line_height) {
        total_height = h;
      }
      while (1) {
        if (cmd_parm == "") {
          break;
        }
        get_text_newline(cmd_parm, new_str, &x1, &y1, &w, &h);
        if (h <= line_height) {
          total_height += line_height;
        }
      }
      // Serial.printf("total_height=%d", total_height);
      if (total_height > led_matrix.height()) {
        break;
      }
      y = (led_matrix.height() - total_height) / 2;
      led_matrix.setCursor(0, y);

      // display all text line in screen center alignment mode
      cmd_parm             = tmp_str;
      bool first_line_flag = true;
      while (1) {
        if (cmd_parm == "") {
          break;
        }
        get_text_newline(cmd_parm, new_str, &x1, &y1, &w, &h);
        if ((first_line_flag) && (current_font_inf != NULL)) {
          first_line_flag = false;
          y += h - 1;
          led_matrix.setCursor(0, y);
        }
        if (h <= line_height) {
          if (get_textaligncenter_cursor(x1, y1, w, h, &x, &y)) {
            led_matrix.setCursor(x, y);
            led_matrix.println(new_str);
          }
        }
      }
    } else if (cmd_type == "IM") { // load an image
      uint8_t x = led_matrix.getCursorX();
      uint8_t y = led_matrix.getCursorY();
      drawBmpFromFile(cmd_parm, x, y);
    } else if (cmd_type == "CI") { // draw a circular
      CIRCLE_DESC circle_par;
      if (get_circle_par(cmd_parm, &circle_par)) {
        uint16_t color16 = led_matrix.color565(circle_par.color.r, circle_par.color.g, circle_par.color.b);
        if ((circle_par.pos.x >= circle_par.radius) && (circle_par.pos.y >= circle_par.radius)) {
          led_matrix.fillCircle(circle_par.pos.x, circle_par.pos.y, circle_par.radius, color16);
        }
      }
    } else if (cmd_type == "RT") { // draw a rectanger
      RECT_DESC rect_par;
      if (get_rectangle_par(cmd_parm, &rect_par)) {
        uint16_t color16 = led_matrix.color565(rect_par.color.r, rect_par.color.g, rect_par.color.b);
        led_matrix.fillRect(rect_par.pos.x, rect_par.pos.y, rect_par.width, rect_par.height, color16);
      }
    } else if (cmd_type == "HP") { // get the width and height of a string
      int16_t x1, y1;
      uint16_t w, h;
      int16_t x = led_matrix.getCursorX();
      int16_t y = led_matrix.getCursorY();
      led_matrix.getTextBounds(cmd_parm, x, y, &x1, &y1, &w, &h);
      SerialCommand.printf("x=%d,y=%d,x1=%d,y1=%d,w=%d,h=%d\n", x, y, x1, y1, w, h);
    } else if (cmd_type == "FS") { // flashing 4 rectangles
      FLASHER_DESC fs_par;
      uint16_t color16 = led_matrix.color565(255, 128, 0);
      if (get_fs_par(cmd_parm, &fs_par)) {
        uint16_t repeat_id = fs_par.times;
        do {
          led_matrix.fillRect(0, 0, 14, 14, 0);         // rect1 off
          led_matrix.fillRect(82, 114, 14, 14, 0);      // rect4 off
          led_matrix.fillRect(82, 0, 14, 14, color16);  // rect2 on
          led_matrix.fillRect(0, 114, 14, 14, color16); // rect3 on
          if (display_delay(fs_par.delay)) {
            break;
          }

          led_matrix.fillRect(0, 0, 14, 14, color16);    // rect1 on
          led_matrix.fillRect(82, 114, 14, 14, color16); // rect4 on
          led_matrix.fillRect(82, 0, 14, 14, 0);         // rect2 off
          led_matrix.fillRect(0, 114, 14, 14, 0);        // rect3 off
          if (display_delay(fs_par.delay)) {
            break;
          }

          repeat_id--;
        } while (repeat_id);
      }
    } else if (cmd_type == "DL") {
      int delay_tm = cmd_parm.toInt();
      display_delay((uint32_t)delay_tm);
    }

    bool load_flag = true;
    while (true) {
      if (isStopCondition()) {
        break;
      }

      LED_COMMAND_DESCRIPTION *command_desc_next = (LED_COMMAND_DESCRIPTION *)(command_desc_curr->qe_next);
      if (command_desc_next != NULL) {
        delete command_desc_curr;
        command_desc_curr = command_desc_next;
        break;
      } else {
        if (load_flag) {
          load_flag = false;
          /*for a command list without D command - ex:"CL;P:Hello;"*/
          led_matrix.showDMABuffer(true);
        } else {
          vTaskDelay(50 / portTICK_PERIOD_MS);
        }
      }
    }

    if (isStopCondition()) {
      break;
    }
  }
  clear_command_desc(command_desc_curr);
}

bool get_text_start_point(uint16_t width, uint16_t height, int16_t *x, int16_t *y, int16_t y_offset) {
  if ((width <= led_matrix.width()) && (height <= led_matrix.height())) {
    *x = (led_matrix.width() - width) / 2;
    *y = (led_matrix.height() - height) / 2 + y_offset;
    return true;
  }
  return false;
}
