#include <Arduino.h>
#include <ESP32-ICND2153-I2S-DMA.h>
#include "icnd2153_driver.h"
#include <Fonts/FreeSansBold12pt7b.h>

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
  //to test DAM, led_matrix.begin(true);
  led_matrix.begin();
  prev_tm = millis();
  load_tm = millis();
}

/* current log result: pass_tm - 24 or 25ms = (10 or 11) + 14*/
void loop() {   
  unsigned long pass_tm, start_tm;
  if(led_matrix.IsRelaseBuffer()){
    start_tm = millis();
    pass_tm = start_tm - prev_tm;
    prev_tm = start_tm;

    /*user screen*/
    // led_matrix.load_test_screen();

    if((millis() - load_tm) > 5000){
      load_tm = millis();
      switch(frame_id){
        case 0:
          led_matrix.fillScreenRGB888(255, 0, 0); frame_id = 1; break;
        case 1:
          led_matrix.fillScreenRGB888(0, 255, 0); frame_id = 2; break;
        case 2:
          led_matrix.fillScreenRGB888(0, 0, 255); frame_id = 0; break;
      }
    }

    Serial.printf("update period: %d, user screen load time:%d\n", pass_tm, millis() - start_tm);
    // Serial.printf("update: %d, load:%d\n", pass_tm, millis() - start_tm);
    led_matrix.showDMABuffer();
  }
}

