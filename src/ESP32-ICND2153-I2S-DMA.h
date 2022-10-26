#ifndef _ESP32_ICND2153_I2S_DMA_
#define _ESP32_ICND2153_I2S_DMA_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "esp_heap_caps.h"
#include "esp32_i2s_parallel.h"
#include "icnd2153_driver.h"

#ifdef USE_GFX_ROOT
#include "GFX.h" // Adafruit GFX core class -> https://github.com/mrfaptastic/GFX_Root
#else
#include "Adafruit_GFX.h" // Adafruit class with all the other stuff
#endif

/* Enable serial debugging of the library, to see how memory is allocated etc. */
#define SERIAL_DEBUG 0

#define PANEL_WIDTH 64
#define PANEL_HEIGHT 32
#define DOUBLE_ROWS 16 //(PANEL_HEIGHT/2)
#define PANEL_CHAIN 3

#define MATRIX_WIDTH 96  /*Visible LED Matrix width*/
#define MATRIX_HEIGHT 64 /*Visible LED Matrix height*/

// Panel Upper half RGB (numbering according to order in DMA gpio_bus configuration)
#define BIT_R1 (1 << 0)
#define BIT_G1 (1 << 1)
#define BIT_B1 (1 << 2)

// Panel Lower half RGB
#define BIT_R2 (1 << 3)
#define BIT_G2 (1 << 4)
#define BIT_B2 (1 << 5)

// Panel Control Signals
#define BIT_LAT (1 << 6)
#define BIT_OE (1 << 7)

// Panel GPIO Pin Addresses (A, B, C, D etc..)
#define BIT_A (1 << 8)
#define BIT_B (1 << 9)
#define BIT_C (1 << 10)

#define BIT_R12 (BIT_R1 | BIT_R2)
#define BIT_G12 (BIT_G1 | BIT_G2)
#define BIT_B12 (BIT_B1 | BIT_B2)
#define BIT_COLOR (BIT_R12 | BIT_G12 | BIT_B12)
#define BIT_ROW (BIT_A | BIT_B | BIT_C)
/***************************************************************************************/
/* Keep this as is. Do not change.                                                     */
#define ESP32_I2S_DMA_MODE I2S_PARALLEL_BITS_16 // Pump 16 bits out in parallel
#define ESP32_I2S_DMA_STORAGE_TYPE uint16_t     // one uint16_t at a time.
//#define ESP32_I2S_CLOCK_SPEED     (20000000UL)            // @ 20Mhz
#define ESP32_I2S_CLOCK_SPEED (10000000UL) // @ 10Mhz
#define CLKS_DURING_LATCH 0                // Not used.
/***************************************************************************************/

typedef struct RGB24 {
  RGB24() : RGB24(0, 0, 0) {}
  RGB24(uint8_t r, uint8_t g, uint8_t b) {
    red   = r;
    green = g;
    blue  = b;
  }
  RGB24 &operator=(const RGB24 &col);

  uint8_t red;
  uint8_t green;
  uint8_t blue;
} RGB24;

/***************************************************************************************/
// Used by val2PWM
// C/p'ed from https://ledshield.wordpress.com/2012/11/13/led-brightness-to-your-eye-gamma-correction-no/
// Example calculator: https://gist.github.com/mathiasvr/19ce1d7b6caeab230934080ae1f1380e
const uint16_t lumConvTab[] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,
    2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   3,   3,   4,   4,   4,   4,   4,   5,   5,   5,   5,   5,
    6,   6,   6,   6,   6,   7,   7,   7,   7,   8,   8,   8,   8,   9,   9,   9,   10,  10,  10,  11,  11,  11,
    12,  12,  12,  13,  13,  13,  14,  14,  14,  15,  15,  16,  16,  17,  17,  17,  18,  18,  19,  19,  20,  20,
    21,  21,  22,  22,  23,  23,  24,  24,  25,  25,  26,  27,  27,  28,  28,  29,  30,  30,  31,  31,  32,  33,
    33,  34,  35,  35,  36,  37,  38,  38,  39,  40,  41,  41,  42,  43,  44,  45,  45,  46,  47,  48,  49,  50,
    51,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,
    73,  74,  75,  76,  77,  78,  80,  81,  82,  83,  84,  86,  87,  88,  90,  91,  92,  93,  95,  96,  98,  99,
    100, 102, 103, 105, 106, 107, 109, 110, 112, 113, 115, 116, 118, 120, 121, 123, 124, 126, 128, 129, 131, 133,
    134, 136, 138, 139, 141, 143, 145, 146, 148, 150, 152, 154, 156, 157, 159, 161, 163, 165, 167, 169, 171, 173,
    175, 177, 179, 181, 183, 185, 187, 189, 192, 194, 196, 198, 200, 203, 205, 207, 209, 212, 214, 216, 218, 221,
    223, 226, 228, 230, 233, 235, 238, 240, 243, 245, 248, 250, 253, 255, 255};

/***************************************************************************************/
#ifdef USE_GFX_ROOT
class ICND2153_I2S_DMA : public GFX {
#else
class ICND2153_I2S_DMA : public Adafruit_GFX {
#endif

  // ------- PUBLIC -------
public:
  /**
   * ICND2153_I2S_DMA
   */
  ICND2153_I2S_DMA(bool _double_buffer = false)
#ifdef USE_GFX_ROOT
      : GFX(MATRIX_WIDTH, MATRIX_HEIGHT), double_buffering_enabled(_double_buffer) {
#else
      : Adafruit_GFX(MATRIX_WIDTH, MATRIX_HEIGHT), double_buffering_enabled(_double_buffer) {
#endif
  }

  bool begin();

  // TODO: Disable/Enable auto buffer flipping (useful for lots of drawPixel usage)...

  // Draw pixels
  virtual void drawPixel(int16_t x, int16_t y, uint16_t color); // overwrite adafruit implementation
  virtual void fillScreen(uint16_t color);                      // overwrite adafruit implementation
  void clearScreen() { fillScreen(0); }
  void fillScreenRGB888(uint8_t r, uint8_t g, uint8_t b);
  void drawPixelRGB565(int16_t x, int16_t y, uint16_t color);
  void drawPixelRGB888(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b);
  void drawPixelRGB24(int16_t x, int16_t y, RGB24 color);
  void drawIcon(int *ico, int16_t x, int16_t y, int16_t cols, int16_t rows);
  void drawBmpFromFile(String filename, uint8_t xMove, uint16_t yMove);
  // Color 444 is a 4 bit scale, so 0 to 15, color 565 takes a 0-255 bit value, so scale up by 255/15 (i.e. 17)!
  uint16_t color444(uint8_t r, uint8_t g, uint8_t b) { return color565(r * 17, g * 17, b * 17); }

  // Converts RGB888 to RGB565
  uint16_t color565(uint8_t r, uint8_t g, uint8_t b); // This is what is used by Adafruit GFX!

  // Converts RGB333 to RGB565
  uint16_t color333(uint8_t r, uint8_t g, uint8_t b); // This is what is used by Adafruit GFX! Not sure why they have a
                                                      // capital 'C' for this particular function.

  // inline void flipDMABuffer()
  // {
  //   if ( !double_buffering_enabled) return;

  //     // Flip to other buffer as the backbuffer. i.e. Graphic changes happen to this buffer (but aren't displayed
  //     until showDMABuffer()) back_buffer_id ^= 1;

  //     #if SERIAL_DEBUG
  //             Serial.printf("Set back buffer to: %d\n", back_buffer_id);
  //     #endif

  //     // Wait before we allow any writing to the buffer. Stop flicker.
  //     while(!i2s_parallel_is_previous_buffer_free()) {}
  // }

  bool IsRelaseBuffer();
  void showDMABuffer(bool wait_flag);

  inline void setPanelBrightness(int b) {
    // Change to set the brightness of the display, range of 1 to matrixWidth (i.e. 1 - 64)
    brightness = b;
  }

  // inline void setMinRefreshRate(int rr)
  // {
  //     min_refresh_rate = rr;
  // }

  int calculated_refresh_rate = 0;

  void MapVisibleToMatrix(uint16_t x, uint16_t y, uint16_t *matrix_x, uint16_t *matrix_y);

  void load_test_screen();

  void i2s_isr_cb_dma2153();

  // ------- PRIVATE -------
private:
  uint16_t *gpioplane_buffer[2];
  uint16_t *bitplane_buffer[2];
  uint16_t *aux_buffer;
  uint16_t *header_buffer;

  // ESP 32 DMA Linked List descriptor
  int desccount_a     = 0;
  int desccount_b     = 0;
  lldesc_t *dmadesc_a = {0};
  lldesc_t *dmadesc_b = {0};

  // ESP32_ICND2153_I2S_DMA functioning
  bool everything_OK = false;
  bool double_buffering_enabled =
      false;              // Do we use double buffer mode? Your project code will have to manually flip between both.
  int back_buffer_id = 0; // If using double buffer, which one is NOT active (ie. being displayed) to write too?
  int brightness = 32; // If you get ghosting... reduce brightness level. 60 seems to be the limit before ghosting on a
                       // 64 pixel wide physical panel for some panels.
  int min_refresh_rate = 99;   // Probably best to leave as is unless you want to experiment. Framerate has an impact on
                               // brightness and also power draw - voltage ripple.
  int lsbMsbTransitionBit = 0; // For possible color depth calculations

  uint8_t gclk_scale;
  uint8_t frame_scan_num;
  uint16_t gclk_pulse_num_per_scan, dma_unit_num;
  int header_length, colorbuffer_length, auxbuffer_length;
  uint8_t colorbuffer_scan_num, auxbuffer_scan_num;
  uint8_t colorbuffer_dma_descount, auxbuffer_dma_descount;
  /* Calculate the memory available for DMA use, do some other stuff, and allocate accordingly */
  bool allocateDMAmemory();
  void clearDMAmemory();
  // uint32_t ValueAt(int double_row, int column, int bit);
  uint16_t ValueAt_t(int double_row, int column);

  /* Setup the DMA Link List chain and initiate the ESP32 DMA engine */
  void configureDMA(int r1_pin, int g1_pin, int b1_pin, int r2_pin, int g2_pin, int b2_pin, int a_pin, int b_pin,
                    int c_pin, int lat_pin, int oe_pin, int clk_pin); // Get everything setup. Refer to the .c file

  /* Update a specific pixel in the DMA buffer to a colour */
  void updateMatrixDMABuffer(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue);

  /* Update the entire DMA buffer (aka. The RGB Panel) a certain colour (wipe the screen basically) */
  void updateMatrixDMABuffer(uint8_t red, uint8_t green, uint8_t blue);

  // Non luminance correction. TODO: consider getting rid of this.
  inline uint16_t DirectMapColor(uint8_t c) {
    // simple scale down the color value
    c = c * brightness / 100;
    return ((uint16_t)c << 8);
  }
  void MapColors(uint8_t r, uint8_t g, uint8_t b, uint16_t *red, uint16_t *green, uint16_t *blue);

}; // end Class header

extern ICND2153_I2S_DMA led_matrix;

/***************************************************************************************/
// https://stackoverflow.com/questions/5057021/why-are-c-inline-functions-in-the-header
/* 2. functions declared in the header must be marked inline because otherwise, every translation unit which includes
 * the header will contain a definition of the function, and the linker will complain about multiple definitions (a
 * violation of the One Definition Rule). The inline keyword suppresses this, allowing multiple translation units to
 * contain (identical) definitions. */
inline void ICND2153_I2S_DMA::drawPixel(int16_t x, int16_t y, uint16_t color) // adafruit virtual void override
{
  drawPixelRGB565(x, y, color);
}

inline void ICND2153_I2S_DMA::fillScreen(uint16_t color) // adafruit virtual void override
{
  uint8_t r = ((((color >> 11) & 0x1F) * 527) + 23) >> 6;
  uint8_t g = ((((color >> 5) & 0x3F) * 259) + 33) >> 6;
  uint8_t b = (((color & 0x1F) * 527) + 23) >> 6;

  updateMatrixDMABuffer(r, g, b); // the RGB only (no pixel coordinate) version of 'updateMatrixDMABuffer'
}

inline void ICND2153_I2S_DMA::fillScreenRGB888(uint8_t r, uint8_t g, uint8_t b) // adafruit virtual void override
{
  updateMatrixDMABuffer(r, g, b);
}

// For adafruit
inline void ICND2153_I2S_DMA::drawPixelRGB565(int16_t x, int16_t y, uint16_t color) {
  uint8_t r = ((((color >> 11) & 0x1F) * 527) + 23) >> 6;
  uint8_t g = ((((color >> 5) & 0x3F) * 259) + 33) >> 6;
  uint8_t b = (((color & 0x1F) * 527) + 23) >> 6;

  updateMatrixDMABuffer(x, y, r, g, b);
}

inline void ICND2153_I2S_DMA::drawPixelRGB888(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b) {
  updateMatrixDMABuffer(x, y, r, g, b);
}

inline void ICND2153_I2S_DMA::drawPixelRGB24(int16_t x, int16_t y, RGB24 color) {
  updateMatrixDMABuffer(x, y, color.red, color.green, color.blue);
}

// Pass 8-bit (each) R,G,B, get back 16-bit packed color
// https://github.com/squix78/ILI9341Buffer/blob/master/ILI9341_SPI.cpp
inline uint16_t ICND2153_I2S_DMA::color565(uint8_t r, uint8_t g, uint8_t b) {
  /*
    Serial.printf("Got r value of %d\n", r);
    Serial.printf("Got g value of %d\n", g);
    Serial.printf("Got b value of %d\n", b);
    */

  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// Promote 3/3/3 RGB to Adafruit_GFX 5/6/5 RRRrrGGGgggBBBbb
inline uint16_t ICND2153_I2S_DMA::color333(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0x7) << 13) | ((r & 0x6) << 10) | ((g & 0x7) << 8) | ((g & 0x7) << 5) | ((b & 0x7) << 2) |
         ((b & 0x6) >> 1);
}

inline void ICND2153_I2S_DMA::drawIcon(int *ico, int16_t x, int16_t y, int16_t cols, int16_t rows) {
  /*  drawIcon draws a C style bitmap.
  //  Example 10x5px bitmap of a yellow sun
  //
    int half_sun [50] = {
        0x0000, 0x0000, 0x0000, 0xffe0, 0x0000, 0x0000, 0xffe0, 0x0000, 0x0000, 0x0000,
        0x0000, 0xffe0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffe0, 0x0000,
        0x0000, 0x0000, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0x0000, 0x0000, 0x0000,
        0xffe0, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0x0000, 0xffe0,
        0x0000, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0x0000, 0x0000,
    };

    ICND2153_I2S_DMA matrix;

    matrix.drawIcon (half_sun, 0,0,10,5);
  */

  int i, j;
  for (i = 0; i < rows; i++) {
    for (j = 0; j < cols; j++) {
      drawPixelRGB565(x + j, y + i, ico[i * cols + j]);
    }
  }
}

// static inline uint16_t CIEMapColor(uint8_t brightness, uint8_t c) {
//   static ColorLookup *luminance_lookup = CreateLuminanceCIE1931LookupTable();
//   return luminance_lookup[brightness - 1].color[c];
// }

inline void ICND2153_I2S_DMA::MapColors(uint8_t r, uint8_t g, uint8_t b, uint16_t *red, uint16_t *green,
                                        uint16_t *blue) {
  // if (do_luminance_correct_) {
  //   *red   = CIEMapColor(r);
  //   *green = CIEMapColor(g);
  //   *blue  = CIEMapColor(b);
  // } else {
  *red   = DirectMapColor(r);
  *green = DirectMapColor(g);
  *blue  = DirectMapColor(b);
  // }

  // if (inverse_color_) {
  //   *red = ~(*red);
  //   *green = ~(*green);
  //   *blue = ~(*blue);
  // }
}

#endif
