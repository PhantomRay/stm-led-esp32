#include <Arduino.h>
#include "command_parsing.h"
#include "display.h"

uint32_t frame_id = 0;

unsigned long prev_tm = 0;
unsigned long load_tm = 0;

void setup() {
  // put your setup code here, to run once:
  delay(2000);
  Serial.begin(115200);
  delay(100);
  Serial.println("I2S DMA.");

  /*register callback for I2S DMA interrupt*/
  // setShiftCompleteCallback(i2s_isr_cb);
  // to test DAM, led_matrix.begin(true);
  display_init();

  // xTaskCreate(&display_task, "command_task", configMINIMAL_STACK_SIZE, NULL, 5, NULL);
  command_init();
  xTaskCreate(&command_task, "command_task", 4096 /*configMINIMAL_STACK_SIZE*/, NULL, 5, NULL);

  prev_tm = millis();
  load_tm = millis();

  // command_parsing();
}

/* current log result: pass_tm - 24 or 25ms = (10 or 11) + 14*/
void loop() {
  // unsigned long pass_tm, start_tm;

  //   // led_matrix.load_test_screen();
  //   // led_matrix.showDMABuffer(true);

  //   if((millis() - load_tm) > 5000){
  //     load_tm = millis();
  //     switch(frame_id){
  //       case 0:
  //         led_matrix.fillScreenRGB888(255, 0, 0); frame_id = 1; break;
  //       case 1:
  //         led_matrix.fillScreenRGB888(0, 255, 0); frame_id = 2; break;
  //       case 2:
  //         led_matrix.fillScreenRGB888(0, 0, 255); frame_id = 0; break;
  //     }

  //     led_matrix.showDMABuffer(true);
  //   }
}
