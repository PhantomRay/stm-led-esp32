#include "SPIFFS.h"
#include "display.h"
#include "ESP32-ICND2153-I2S-DMA.h"

#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeSerif12pt7b.h>

LED_COMMAND_DESCRIPTION *display_command = NULL;

void display_init() {
  led_matrix.begin();
  display_command = NULL;
  command_init();
}

static bool get_integer(String color_str, uint8_t color[], uint8_t num) {
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

void update_display_param(LED_COMMAND_DESCRIPTION *cmd_desc_first) {
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
    } else if (cmd_type == "I") {
      uint8_t x = led_matrix.getCursorX();
      uint8_t y = led_matrix.getCursorY();
      drawBmpFromFile(cmd_parm, x, y);
    } else if (cmd_type == "D") {
      led_matrix.showDMABuffer(true);
      vTaskDelay(cmd_parm.toInt() / portTICK_PERIOD_MS); // delay(cmd_parm.toInt());
    } else if (cmd_type == "FL") {
    }

    command_desc_tmp = (LED_COMMAND_DESCRIPTION *)(command_desc_tmp->qe_next);
  }
}
