#include "ESP32-ICND2153-I2S-DMA.h"
#include "icnd2153_driver.h"

ICND2153_I2S_DMA led_matrix(false);

void IRAM_ATTR i2s_isr_cb() { led_matrix.i2s_isr_cb_dma2153(); }

volatile bool new_frame_ready_flag = false, framebuffer_release_flag = true;
void IRAM_ATTR ICND2153_I2S_DMA::i2s_isr_cb_dma2153() {
  if (new_frame_ready_flag) {
    framebuffer_release_flag = false;
    new_frame_ready_flag     = false;

    dmadesc_b[desccount_b - 1].qe.stqe_next = (lldesc_t *)&dmadesc_b[0];
  } else {
    // if(framebuffer_release_flag == false){
    //   framebuffer_release_flag = true;
    // }
    framebuffer_release_flag = true;
  }
}

bool ICND2153_I2S_DMA::IsRelaseBuffer() {
  if ((new_frame_ready_flag == false) && (framebuffer_release_flag == true)) {
    return true;
  }
  return false;
}

/*when wait_flag is true, wait until framebuffer is released*/
// if you use false for wait_flag, you must call IsRelaseBuffer() before updating a frame buffer.
void ICND2153_I2S_DMA::showDMABuffer(bool wait_flag) {
  dmadesc_b[desccount_b - 1].qe.stqe_next = (lldesc_t *)&dmadesc_a[0];
  new_frame_ready_flag                    = true;

  if (wait_flag) {
    // Serial.println("framebuffer is locked");
    unsigned long pass_tm, start_tm;
    start_tm = millis();
    while (!IsRelaseBuffer()) {
    };
    // Serial.printf("framebuffer is released. locked time: %d\n", millis() - start_tm);
  }

  // {
  //     if (!double_buffering_enabled) return;
  //     #if SERIAL_DEBUG
  //             Serial.printf("Showtime for buffer: %d\n", back_buffer_id);
  //     #endif
  //     i2s_parallel_flip_to_buffer(&I2S1, back_buffer_id);
  //     // Wait before we allow any writing to the buffer. Stop flicker.
  //     while(!i2s_parallel_is_previous_buffer_free()) {}
  // }
}

/* Propagate the DMA pin configuration, or use compiler defaults */
bool ICND2153_I2S_DMA::begin() {
  InitICND2153(8 * PANEL_CHAIN);
  /*--- header for Vertical sync ---*/
  header_length = 4; // real header length is 3(Vertical sync) half word, but it must be a multiple of the word length.

  gclk_scale = 2; // when OE signal(GCLK for ICND2153) is merged with data(color) signals, 2. If other, 1
  /*--- The total number of GCLKs is frame_scan_num * gclk_pulse_num_per_scan ---*/
  frame_scan_num          = 64;
  gclk_pulse_num_per_scan = 138 * gclk_scale * 8;
  /*dma buffer for real color region*/
  colorbuffer_length   = PANEL_WIDTH * (PANEL_HEIGHT / 2) * 16 /*bits/pixel*/ * PANEL_CHAIN + (4 * gclk_scale);
  colorbuffer_scan_num = colorbuffer_length / gclk_pulse_num_per_scan + 1; // when PANEL_CHAIN=3, 23
  if (colorbuffer_scan_num >= frame_scan_num) {
    Serial.println("DMA buffer parameter error");
    return false;
  }
  // extend dma buffer for real color region
  colorbuffer_length = colorbuffer_scan_num * gclk_pulse_num_per_scan;
  auxbuffer_scan_num = frame_scan_num - colorbuffer_scan_num;
  auxbuffer_length   = gclk_pulse_num_per_scan + (4 * gclk_scale);

  // number of LL description
  dma_unit_num             = gclk_pulse_num_per_scan >> 1; // because gclk_pulse_num_per_scan * sizeof(uint16_t) > 4092
  colorbuffer_dma_descount = colorbuffer_scan_num * 2;     // this is same as (colorbuffer_length / dma_unit_num)
  auxbuffer_dma_descount   = auxbuffer_scan_num * 2;

  everything_OK            = false;
  double_buffering_enabled = false;

  if (!allocateDMAmemory()) {
    return false;
  } // couldn't even get the basic ram required.

  brightness = 50;
  // load_test_screen();

  // Flush the DMA buffers prior to configuring DMA - Avoid visual artefacts on boot.
  // clearScreen(); // Must fill the DMA buffer with the initial output bit sequence or the panel will display garbage
  // flipDMABuffer(); // flip to backbuffer 1
  // clearScreen(); // Must fill the DMA buffer with the initial output bit sequence or the panel will display garbage
  // flipDMABuffer(); // backbuffer 0

  // Setup the ESP32 DMA Engine. Sprite_TM built this stuff.
  configureDMA(PIN_R1, PIN_G1, PIN_B1, PIN_R2, PIN_G2, PIN_B2, PIN_A, PIN_B, PIN_C, PIN_LAT, PIN_OE,
               PIN_CLK); // DMA and I2S configuration and setup

#if SERIAL_DEBUG
  if (!everything_OK) Serial.println("ICND2153_I2S_DMA::begin() failed.");
#endif

  /*register callback*/
  setShiftCompleteCallback(i2s_isr_cb);

  showDMABuffer(true);
  return everything_OK;
}

uint8_t val2PWM(int val) {
  if (val < 0) val = 0;
  if (val > 255) val = 255;
  return lumConvTab[val];
}

// /*b[0..15] - b0 is MSB(2^15) and b15 is LSB(2^0).*/
// uint32_t ICND2153_I2S_DMA::ValueAt(int double_row, int column, int bit) {
//   uint32_t offset;
//   uint8_t row_id, row_rem, col_id, col_rem;
//   uint8_t panel_num = PANEL_CHAIN; //the number of chain
//   uint8_t panel_id = column / 64;
//   column = column % 64;

//   row_id = double_row % 8;
//   row_rem = (double_row < 8)? 1:0;
//   col_id = (row_rem * 4) + column / 16;
//   col_rem = column % 16;
//   offset = (row_id * 16 * 8 * panel_num + col_rem * 8 * panel_num + panel_id * 8 + col_id) * 16 + bit;
//   return offset;
// }
uint16_t ICND2153_I2S_DMA::ValueAt_t(int double_row, int column) {
  uint16_t offset;
  uint8_t row_id, row_rem, col_id, col_rem;
  uint8_t panel_num = PANEL_CHAIN; // the number of chain
  uint8_t panel_id  = column / 64;
  column            = column % 64;

  row_id  = double_row % 8;
  row_rem = (double_row < 8) ? 1 : 0;
  col_id  = (row_rem * 4) + column / 16;
  col_rem = column % 16;
  offset  = (row_id * 16 * 8 * panel_num + col_rem * 8 * panel_num + panel_id * 8 + col_id);
  return offset;
}

void ICND2153_I2S_DMA::clearDMAmemory() {
  /*--------------------------*/
  /*--- set header buffer  ---*/
  /*--------------------------*/
  header_buffer[1] = 0;
  header_buffer[0] = BIT_LAT;
  header_buffer[2] = BIT_LAT;
  header_buffer[3] = BIT_LAT;

  /*--------------------------*/
  /*---  set color buffer  ---*/
  /*--------------------------*/
  /*set SCAN line*/
  uint16_t row_lookup[8];
  for (int i = 0; i < 8; ++i) {
    uint16_t row_address = (i & 0x01) ? BIT_A : 0;
    row_address |= (i & 0x02) ? BIT_B : 0;
    row_address |= (i & 0x04) ? BIT_C : 0;
    row_lookup[i] = row_address;
  }

  uint16_t *pbuf0 = gpioplane_buffer[0];
  // uint16_t *pbuf1 = gpioplane_buffer[1];
  for (uint8_t scan_frame = 0; scan_frame < colorbuffer_scan_num; scan_frame++) {
    for (uint8_t scan_line = 0; scan_line < 8; scan_line++) {
      for (uint8_t gclk_cnt = 0; gclk_cnt < 138; gclk_cnt++) {
        *pbuf0++ = row_lookup[scan_line];
        *pbuf0++ = row_lookup[scan_line] | BIT_OE;
      }
    }
  }

  /*set latch(strobe)*/
  uint32_t pixel_pos;
  for (int y = 0; y < 8; ++y) {
    for (int x = 64 * PANEL_CHAIN - 16; x < 64 * PANEL_CHAIN; ++x) {
      pixel_pos = ((uint32_t)ValueAt_t(y, x) << 4) + 14;
      bitplane_buffer[0][pixel_pos] |= BIT_LAT;
    }
  }
  /*--------------------------*/
  /*---set auxiliary buffer---*/
  /*--------------------------*/
  uint16_t *pbuf = aux_buffer;
  for (uint8_t scan_line = 0; scan_line < 8; scan_line++) {
    for (uint8_t gclk_cnt = 0; gclk_cnt < 138; gclk_cnt++) {
      *pbuf++ = row_lookup[scan_line];
      *pbuf++ = row_lookup[scan_line] | BIT_OE;
    }
  }
  for (uint8_t gclk_cnt = 0; gclk_cnt < 4; gclk_cnt++) {
    *pbuf++ = row_lookup[0];
    *pbuf++ = row_lookup[0] | BIT_OE;
  }
}

bool ICND2153_I2S_DMA::allocateDMAmemory() {
  /***
   * Step 1: Look at the overall DMA capable memory for the DMA FRAMEBUFFER data only (not the DMA linked list
   * descriptors yet) and do some pre-checks.
   */
  {
    size_t _frame_buffer_memory_required =
        sizeof(uint16_t) * (colorbuffer_length + auxbuffer_length); // sizeof(frameStruct) * _num_frame_buffers;
    size_t _total_dma_capable_memory_reserved = 0;

// 1. Calculate the amount of DMA capable memory that's actually available
#if SERIAL_DEBUG
    // Serial.println("DMA memory blocks available before any malloc's: ");
    // heap_caps_print_heap_info(MALLOC_CAP_DMA);

    Serial.printf("We're going to need %d bytes of SRAM just for the frame buffer(s).\r\n",
                  _frame_buffer_memory_required);
    Serial.printf("The total amount of DMA capable SRAM memory is %d bytes.\r\n",
                  heap_caps_get_free_size(MALLOC_CAP_DMA));
    Serial.printf("Largest DMA capable SRAM memory block is %d bytes.\r\n",
                  heap_caps_get_largest_free_block(MALLOC_CAP_DMA));

#endif

    // Can we potentially fit the framebuffer into the DMA capable memory that's available?
    if (heap_caps_get_free_size(MALLOC_CAP_DMA) < _frame_buffer_memory_required) {
#if SERIAL_DEBUG
      Serial.printf("######### Insufficient memory for requested resolution. Reduce MATRIX_COLOR_DEPTH and try "
                    "again.\r\n\tAdditional %d bytes of memory required.\r\n\r\n",
                    (_frame_buffer_memory_required - heap_caps_get_free_size(MALLOC_CAP_DMA)));
#endif

      return false;
    }

    gpioplane_buffer[0] = (uint16_t *)heap_caps_malloc((sizeof(uint16_t) * colorbuffer_length), MALLOC_CAP_DMA);
    if (gpioplane_buffer[0] == NULL) {
      Serial.println("buffer0 allocation fail");
      return false;
    }
    // gpioplane_buffer[1] = (uint16_t*)heap_caps_malloc((sizeof(uint16_t) * colorbuffer_length) , MALLOC_CAP_DMA);
    // if(gpioplane_buffer[1] == NULL){
    //   Serial.println("buffer1 allocation fail");
    //   return false;
    // }
    header_buffer =
        (uint16_t *)heap_caps_malloc((sizeof(uint16_t) * (header_length + auxbuffer_length)), MALLOC_CAP_DMA);
    aux_buffer = &header_buffer[header_length];
    if (aux_buffer == NULL) {
      Serial.println("aux_buffer allocation fail");
      return false;
    }
    bitplane_buffer[0] = &gpioplane_buffer[0][4 * gclk_scale];
    // bitplane_buffer[1] = &gpioplane_buffer[1][4 * gclk_scale];
    Serial.println("DMA Data buffer allocation Ok.");

    clearDMAmemory();

#if SERIAL_DEBUG
    Serial.println("DMA capable memory map available after malloc's: ");
    heap_caps_print_heap_info(MALLOC_CAP_DMA);
    delay(1000);
#endif
  } // end of Step 1

  /***
   * Step 2: Allocate memory for DMA linked list, linking up each framebuffer in sequence for GPIO output.
   */
  {
    // 1 is for header of frame(Vsync), frame_scan_num * 2 = colorbuffer_dma_descount + auxbuffer_dma_descount, and
    // double
    desccount_a = (colorbuffer_dma_descount + 1) * 2; //(frame_scan_num * 2 + 1)*2;
    dmadesc_a   = (lldesc_t *)heap_caps_malloc(desccount_a * sizeof(lldesc_t), MALLOC_CAP_DMA);

    desccount_b = frame_scan_num * 2 + 1;
    dmadesc_b   = (lldesc_t *)heap_caps_malloc(desccount_b * sizeof(lldesc_t), MALLOC_CAP_DMA);

    size_t _dma_linked_list_memory_required = (desccount_a + desccount_b) * sizeof(lldesc_t);
  }

  // Just os we know
  everything_OK = true;

  return true;

} // end initMatrixDMABuffer()

void ICND2153_I2S_DMA::configureDMA(int r1_pin, int g1_pin, int b1_pin, int r2_pin, int g2_pin, int b2_pin, int a_pin,
                                    int b_pin, int c_pin, int lat_pin, int oe_pin, int clk_pin) {
#if SERIAL_DEBUG
  Serial.println("configureDMA(): Starting configuration of DMA engine.\r\n");
#endif

  /*color buffer link descriptors*/
  lldesc_t *previous_dmadesc_a     = 0;
  int current_dmadescriptor_offset = 0;
  for (int repeat_id = 0; repeat_id < 2; repeat_id++) {
    link_dma_desc(&dmadesc_a[current_dmadescriptor_offset], previous_dmadesc_a, header_buffer,
                  sizeof(uint16_t) * header_length);
    previous_dmadesc_a = &dmadesc_a[current_dmadescriptor_offset];
    current_dmadescriptor_offset++;
    uint16_t *pbuf = gpioplane_buffer[0];
    for (int scan_id = 0; scan_id < colorbuffer_dma_descount; scan_id++) {
      link_dma_desc(&dmadesc_a[current_dmadescriptor_offset], previous_dmadesc_a, pbuf,
                    sizeof(uint16_t) * dma_unit_num);
      previous_dmadesc_a = &dmadesc_a[current_dmadescriptor_offset];
      current_dmadescriptor_offset++;
      pbuf += dma_unit_num;
    }
    /*use following code when desccount_a = (frame_scan_num * 2 + 1)*2;*/
    // for(int scan_id = 0; scan_id < auxbuffer_dma_descount/2; scan_id++) {
    //   link_dma_desc(&dmadesc_a[current_dmadescriptor_offset], previous_dmadesc_a, aux_buffer, sizeof(uint16_t) *
    //   dma_unit_num); previous_dmadesc_a = &dmadesc_a[current_dmadescriptor_offset]; current_dmadescriptor_offset++;
    //   link_dma_desc(&dmadesc_a[current_dmadescriptor_offset], previous_dmadesc_a, &aux_buffer[dma_unit_num],
    //   sizeof(uint16_t) * dma_unit_num); previous_dmadesc_a = &dmadesc_a[current_dmadescriptor_offset];
    //   current_dmadescriptor_offset++;
    // }
  }
  if (desccount_a != current_dmadescriptor_offset) {
    Serial.printf(
        "configureDMA(): ERROR! Color buffer Expected descriptor count of %d != actual DMA descriptors of %d!\r\n",
        desccount_a, current_dmadescriptor_offset);
  }

  /*auxiliary buffer link descriptors*/
  lldesc_t *previous_dmadesc_b = 0;
  current_dmadescriptor_offset = 0;
  link_dma_desc(&dmadesc_b[current_dmadescriptor_offset], previous_dmadesc_b, header_buffer,
                sizeof(uint16_t) * header_length);
  previous_dmadesc_b = &dmadesc_b[current_dmadescriptor_offset];
  current_dmadescriptor_offset++;
  for (int scan_id = 0; scan_id < frame_scan_num; scan_id++) {
    link_dma_desc(&dmadesc_b[current_dmadescriptor_offset], previous_dmadesc_b, aux_buffer,
                  sizeof(uint16_t) * dma_unit_num);
    previous_dmadesc_b = &dmadesc_b[current_dmadescriptor_offset];
    current_dmadescriptor_offset++;
    link_dma_desc(&dmadesc_b[current_dmadescriptor_offset], previous_dmadesc_b, &aux_buffer[dma_unit_num],
                  sizeof(uint16_t) * dma_unit_num);
    previous_dmadesc_b = &dmadesc_b[current_dmadescriptor_offset];
    current_dmadescriptor_offset++;
  }
  if (desccount_b != current_dmadescriptor_offset) {
    Serial.printf(
        "configureDMA(): ERROR! Auxiliary buffer Expected descriptor count of %d != actual DMA descriptors of %d!\r\n",
        desccount_b, current_dmadescriptor_offset);
  }
#if SERIAL_DEBUG
  Serial.printf("configureDMA(): Configured LL structure. %d DMA Linked List descriptors populated.\r\n",
                current_dmadescriptor_offset);
#endif

  // in following code, when dmadesc_a[desccount_a-1].eof = dmadesc_b[desccount_b-1].eof = 1, 2 interrupts happen
  dmadesc_a[desccount_a - 1].eof          = 1;
  dmadesc_a[desccount_a - 1].qe.stqe_next = (lldesc_t *)&dmadesc_b[0];
  dmadesc_b[desccount_b - 1].eof          = 1;
  dmadesc_b[desccount_b - 1].qe.stqe_next = (lldesc_t *)&dmadesc_b[0];

  Serial.printf("Performing I2S setup.\n");

  i2s_parallel_config_t cfg = {
      .gpio_bus = {r1_pin, g1_pin, b1_pin, r2_pin, g2_pin, b2_pin, lat_pin, oe_pin, a_pin, b_pin, c_pin, -1, -1, -1, -1,
                   -1},
      .gpio_clk = clk_pin,
      .clkspeed_hz =
          ESP32_I2S_CLOCK_SPEED,  // ESP32_I2S_CLOCK_SPEED,  // formula used is 80000000L/(cfg->clkspeed_hz + 1), must
                                  // result in >=2.  Acceptable values 26.67MHz, 20MHz, 16MHz, 13.34MHz...
      .bits = ESP32_I2S_DMA_MODE, // ESP32_I2S_DMA_MODE,
      .bufa = 0,
      .bufb = 0,
      desccount_a,
      desccount_b,
      dmadesc_a,
      dmadesc_b};

  // Setup I2S
  i2s_parallel_setup_without_malloc(&I2S1, &cfg);

#if SERIAL_DEBUG
  Serial.println("configureDMA(): DMA configuration completed on I2S1.\r\n");
#endif

#if SERIAL_DEBUG
  Serial.println("DMA Memory Map after DMA LL allocations: ");
  heap_caps_print_heap_info(MALLOC_CAP_DMA);

  delay(1000);
#endif

} // end initMatrixDMABuff

/* Update a specific co-ordinate in the DMA buffer */
/* Original version were we re-create the bitstream from scratch for each x,y co-ordinate / pixel changed. Slightly
 * slower. */
void ICND2153_I2S_DMA::updateMatrixDMABuffer(int16_t x_coord, int16_t y_coord, uint8_t r, uint8_t g, uint8_t b) {
  uint16_t red, green, blue;
  MapColors(r, g, b, &red, &green, &blue);

  uint16_t matrix_x, matrix_y;
  MapVisibleToMatrix(x_coord, y_coord, &matrix_x, &matrix_y);

  uint16_t r_bits, g_bits, b_bits;
  if (matrix_y >= DOUBLE_ROWS) {
    matrix_y -= DOUBLE_ROWS;
    r_bits = BIT_R2;
    g_bits = BIT_G2;
    b_bits = BIT_B2;
  } else {
    r_bits = BIT_R1;
    g_bits = BIT_G1;
    b_bits = BIT_B1;
  }
  uint16_t designator_mask = ~(r_bits | g_bits | b_bits);
  uint16_t *bits;
  uint32_t offset = (uint32_t)ValueAt_t(matrix_y, matrix_x) << 4;
  bits            = &bitplane_buffer[0][offset];

  for (uint16_t i = 0; i < 16; i++) {
    uint16_t mask       = 0x8000 >> i;
    uint16_t color_bits = 0;
    if (red & mask) color_bits |= r_bits;
    if (green & mask) color_bits |= g_bits;
    if (blue & mask) color_bits |= b_bits;

    if (i % 2) {
      bits[i - 1] = (bits[i - 1] & designator_mask) | color_bits;
    } else {
      bits[i + 1] = (bits[i + 1] & designator_mask) | color_bits;
    }
  }

} // updateMatrixDMABuffer (specific co-ords change)

/* Update the entire buffer with a single specific colour - quicker */
void ICND2153_I2S_DMA::updateMatrixDMABuffer(uint8_t r, uint8_t g, uint8_t b) {
  uint16_t red, green, blue;
  MapColors(r, g, b, &red, &green, &blue);
  // Serial.printf("r=%d,g=%d,b=%d\n",red, green, blue);

  uint16_t designator_mask = ~BIT_COLOR;
  for (uint16_t i = 0; i < 16; i++) {
    uint16_t mask       = 0x8000 >> i;
    uint16_t color_bits = 0;
    if (red & mask) color_bits |= BIT_R12;
    if (green & mask) color_bits |= BIT_G12;
    if (blue & mask) color_bits |= BIT_B12;

    uint16_t *bits;
    if (i % 2) {
      bits = &bitplane_buffer[0][i - 1];
    } else {
      bits = &bitplane_buffer[0][i + 1];
    }

    for (uint16_t pixel_id = 0; pixel_id < 64 * 16 * 3; pixel_id++) {
      *bits = (*bits & designator_mask) | color_bits;
      bits += 16;
    }
  }

} // updateMatrixDMABuffer (full frame paint)

// uint32_t ICND2153_I2S_DMA::pixel_mapper(uint8_t x, uint8_t y){
//   uint16_t matrix_x, matrix_y;
//   MapVisibleToMatrix(x, y, &matrix_x, &matrix_y);
//   if(matrix_y > ROWS_PER_FRAME){

//   }
// }

/*--- pixel mapper ---*/
void ICND2153_I2S_DMA::MapVisibleToMatrix(uint16_t x, uint16_t y, uint16_t *matrix_x, uint16_t *matrix_y) {
  //  ___       ___       ___
  // | O |  -< | O |  -< | O |
  // | ^ |  |  | ^ |  |  | ^ |
  // | I | <-  | I | <-  | I |}- Board connector(HUB75)
  //  ---       ---       ---
  uint16_t x_new, y_new;
  y_new     = (PANEL_HEIGHT - 1) - (x % PANEL_HEIGHT); // rows = 32
  x_new     = (x / PANEL_HEIGHT) * PANEL_WIDTH + y;
  *matrix_x = x_new;
  *matrix_y = y_new;
}

uint32_t mapping_tb[11][2] = {
    {BIT_R1, PIN_MASK_R1}, {BIT_G1, PIN_MASK_G1},  {BIT_B1, PIN_MASK_B1}, {BIT_R2, PIN_MASK_R2},
    {BIT_G2, PIN_MASK_G2}, {BIT_B2, PIN_MASK_B2},  {BIT_A, PIN_MASK_A},   {BIT_B, PIN_MASK_B},
    {BIT_C, PIN_MASK_C},   {BIT_LAT, PIN_MASK_LE}, {BIT_OE, PIN_MASK_OE},
};
inline uint32_t get_gpio(uint16_t val16) {
  uint32_t gpio_val = 0;
  for (int i = 0; i < 11; i++) {
    if (val16 & mapping_tb[i][0]) {
      gpio_val |= mapping_tb[i][1];
    }
  }

  return gpio_val;
}

void ICND2153_I2S_DMA::load_test_screen() {
  /* test fill */
  // fillScreenRGB888(255, 0, 0);

  /* test pixel */
  // updateMatrixDMABuffer(5, 5, 255, 0, 0);

  /* test line */
  // int line_id = 0;
  // uint16_t color;
  // do{
  //   color = color565(255, 0, 0);
  //   drawLine(1, line_id, 94, line_id, color);
  //   line_id += 2;
  //   color = color565(0, 255, 0);
  //   drawLine(1, line_id, 94, line_id, color);
  //   line_id += 2;
  //   color = color565(0, 0, 255);
  //   drawLine(1, line_id, 94, line_id, color);
  //   line_id += 2;
  // }while(line_id < 60);
  //   color = color565(255, 0, 0);
  //   drawLine(1, line_id, 94, line_id, color);
  //   line_id += 2;
  //   color = color565(0, 255, 0);
  //   drawLine(1, line_id, 94, line_id, color);

  /* test font */
  setPanelBrightness(10);
  setTextColor(color565(255, 0, 0));
  setCursor(1, 0);
  println("Hello World!");
  // setFont(&FreeSansBold12pt7b);
  // setTextSize(2);
  setTextColor(color565(0, 255, 0));
  println("ICND2153 Driver");
  setTextColor(color565(0, 0, 255));
  println("abcdefghijklmnop");
  println("ABCDEFGHIJKLMNOP");
  setTextColor(color565(128, 128, 128));
  println("0123456789");
  println("~!@#$%^&*()-_+=|{}[]:;\"<>,.?/");
}
