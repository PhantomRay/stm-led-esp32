#include "SPIFFS.h"
#include "display.h"
#include "ESP32-ICND2153-I2S-DMA.h"

// https://rop.nl/truetype2gfx/
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeSerif12pt7b.h>
#include <Fonts/FreeSerif14pt7b.h>
#include <Fonts/FreeSerif16pt7b.h>
#include <Fonts/FreeSerif18pt7b.h>
#include <Fonts/FreeSerif24pt7b.h>

#include <Fonts/FreeSans46pt7b.h>
#include <Fonts/FreeSansBold46pt7b.h>
#include <Fonts/FreeSerif46pt7b.h>
#include <Fonts/FreeSerifBold46pt7b.h>

void display_init() {
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
    parm_fs->times = parm_string_array[0].toInt();
    parm_fs->delay = parm_string_array[1].toInt();
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
  if ((command_desc_update_flag == true) && (command_desc_stop_flag == true)) {
    return true;
  }
  return false;
}
bool display_delay(uint16_t timeout_ms) {
  led_matrix.showDMABuffer(true);
  bool ret_val = false;
  while (timeout_ms >= 100) {
    if (isStopCondition()) {
      ret_val = true;
      break;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
    timeout_ms -= 100;
  };
  return ret_val;
}

void display_task() {
  while (!command_desc_update_flag) {
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }

  LED_COMMAND_DESCRIPTION *cmd_desc_first = command_desc[current_display_description_id];
  command_desc_update_flag                = false;

  LED_COMMAND_DESCRIPTION *command_desc_tmp = cmd_desc_first;
  String cmd_type;
  String cmd_parm;
  while (command_desc_tmp != NULL) {
    cmd_type = command_desc_tmp->cmd.type;
    cmd_parm = command_desc_tmp->cmd.parameter;

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
      if (cmd_parm == "default") {
        led_matrix.setFont();
      } else {
        led_matrix.setTextSize(1);
        if (cmd_parm == "FreeMono9pt7b") {
          led_matrix.setFont(&FreeMono9pt7b);
        } else if (cmd_parm == "FreeMono12pt7b") {
          led_matrix.setFont(&FreeMono12pt7b);
        } else if (cmd_parm == "FreeSans9pt7b") {
          led_matrix.setFont(&FreeSans9pt7b);
        } else if (cmd_parm == "FreeSans12pt7b") {
          led_matrix.setFont(&FreeSans12pt7b);
        } else if (cmd_parm == "FreeSerif9pt7b") {
          led_matrix.setFont(&FreeSerif9pt7b);
        } else if (cmd_parm == "FreeSerif12pt7b") {
          led_matrix.setFont(&FreeSerif12pt7b);
        } else if (cmd_parm == "FreeSerif14pt7b") {
          led_matrix.setFont(&FreeSerif14pt7b);
        } else if (cmd_parm == "FreeSerif16pt7b") {
          led_matrix.setFont(&FreeSerif16pt7b);
        } else if (cmd_parm == "FreeSerif18pt7b") {
          led_matrix.setFont(&FreeSerif18pt7b);
        } else if (cmd_parm == "FreeSerif24pt7b") {
          led_matrix.setFont(&FreeSerif24pt7b);
        } else if (cmd_parm == "FreeSans46pt7b") {
          led_matrix.setFont(&FreeSans46pt7b);
        } else if (cmd_parm == "FreeSansBold46pt7b") {
          led_matrix.setFont(&FreeSansBold46pt7b);
        } else if (cmd_parm == "FreeSerif46pt7b") {
          led_matrix.setFont(&FreeSerif46pt7b);
        } else if (cmd_parm == "FreeSerifBold46pt7b") {
          led_matrix.setFont(&FreeSerifBold46pt7b);
        } else {
          Serial.println("Font no support.");
        }
      }
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
    } else if (cmd_type == "PT") { // print a string
      led_matrix.println(cmd_parm);
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
      display_delay(delay_tm);
    }
    if (isStopCondition()) {
      break;
    }
    command_desc_tmp = (LED_COMMAND_DESCRIPTION *)(command_desc_tmp->qe_next);
  }
  /*for a command list without D command - ex:"CL;P:Hello;"*/
  led_matrix.showDMABuffer(true);
}

bool get_text_start_point(uint16_t width, uint16_t height, int16_t *x, int16_t *y, int16_t y_offset) {
  if ((width <= led_matrix.width()) && (height <= led_matrix.height())) {
    *x = (led_matrix.width() - width) / 2;
    *y = (led_matrix.height() - height) / 2 + y_offset;
    return true;
  }
  return false;
}
void test_display_text() {
  // GFXfont *curr_font = &FreeSerif46pt7b;
  // led_matrix.setFont(curr_font);
  // led_matrix.setCursor(0, 0);
  // int16_t x1, y1;
  // uint16_t w, h;
  // // bool one_line_flag = true;
  // // for (int i = 0; i < 99; i++) {
  // //   led_matrix.getTextBounds(String(i), 0, 0, &x1, &y1, &w, &h);
  // //   // SerialCommand.printf("%d:x1=%d,y1=%d,w=%d,h=%d\n", i, x1, y1, w, h);
  // //   if (h > 64) {
  // //     SerialCommand.printf("%d:x1=%d,y1=%d,w=%d,h=%d\n", i, x1, y1, w, h);
  // //     one_line_flag = false;
  // //   }
  // // }
  // x1 = (led_matrix.width() - curr_font->glyph->xAdvance) / 2;
  // y1 = (led_matrix.height() - 60) / 2 + 60;
  // for (int i = 0; i < 10; i++) {
  //   led_matrix.fillScreenRGB888(0, 0, 0);
  //   // led_matrix.getTextBounds(String(i), 0, 0, &x1, &y1, &w, &h);
  //   // get_text_start_point(w, h, &x1, &y1, 61);
  //   led_matrix.setCursor(x1, y1);
  //   led_matrix.println(String(i));
  //   led_matrix.showDMABuffer(true);
  //   vTaskDelay(1500 / portTICK_PERIOD_MS);
  // }
  // x1 = (led_matrix.width() - (curr_font->glyph->xAdvance) * 2) / 2;
  // y1 = (led_matrix.height() - 60) / 2 + 60;
  // for (int i = 10; i < 99; i++) {
  //   led_matrix.fillScreenRGB888(0, 0, 0);
  //   // led_matrix.getTextBounds(String(i), 0, 0, &x1, &y1, &w, &h);
  //   // get_text_start_point(w, h, &x1, &y1, 61);
  //   led_matrix.setCursor(x1, y1);
  //   led_matrix.println(String(i));
  //   led_matrix.showDMABuffer(true);
  //   vTaskDelay(1500 / portTICK_PERIOD_MS);
  // }

  /*get a position for "SLOW" and "DOWN"*/
  int16_t x0, x1, y0, y1;
  uint16_t w, h;
  const GFXfont *curr_font = &FreeSerif14pt7b;
  led_matrix.setFont(curr_font);
  led_matrix.setCursor(0, 0);
  led_matrix.getTextBounds("SLOW", 0, 0, &x0, &y0, &w, &h);
  get_text_start_point(w, h * 2 + 10, &x0, &y0, h);
  Serial.printf("SLOW x = %d, y = %d\n", x0, y0);
  led_matrix.setCursor(x0, y0);
  led_matrix.println("SLOW");

  led_matrix.getTextBounds("DOWN", 0, 0, &x1, &y1, &w, &h);
  get_text_start_point(w, h * 2 + 10, &x1, &y1, h);
  y1 = y0 + 10 + h;
  Serial.printf("DOWN x = %d, y = %d\n", x1, y1);
  led_matrix.setCursor(x1, y1);
  led_matrix.println("DOWN");
  led_matrix.showDMABuffer(true);
  vTaskDelay(3000 / portTICK_PERIOD_MS);
}
