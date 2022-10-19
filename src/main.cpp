#include <Arduino.h>
#include <ESP32-ICND2153-I2S-DMA.h>
#include "icnd2153_driver.h"
#include <Fonts/FreeSansBold12pt7b.h>

ICND2153_I2S_DMA led_matrix(false);
uint32_t frame_id = 0;

unsigned long prev_tm0 = 0;
unsigned long prev_tm = 0;
unsigned long max_pass_tm = 0;
void IRAM_ATTR i2c_isr_cb(){
  unsigned long pass_tm, curr_tm;
  curr_tm = millis();
  pass_tm = curr_tm - prev_tm;
  prev_tm = curr_tm;
  frame_id++;
  if(frame_id > 5){
    if(max_pass_tm < pass_tm){
      max_pass_tm = pass_tm;
    }
  }
  // Serial.printf("%d:%dms", frame_id, pass_tm);
}

void setup() {
  // put your setup code here, to run once:
  delay(2000);
  Serial.begin(115200);
  delay(100);
  Serial.println("I2S DMA.");
  
  InitICND2153(8*PANEL_CHAIN);
  /*register callback for I2S DMA interrupt*/
  setShiftCompleteCallback(i2c_isr_cb);
  //to test DAM, led_matrix.begin(true);
  led_matrix.begin(true);

}

void loop() {
  if(led_matrix.dma_mode_flag == false){
    /*for testing Non-DMA mode. for use this, you must not call configureDMA()*/
    unsigned long start_tm = millis();
    // put your main code here, to run repeatedly:
    led_matrix.DumpToMatrix();
    frame_id++;
    unsigned long pass_tm = millis() - start_tm;
    // Serial.printf("%d:%dms", frame_id, pass_tm);
  }
  else{//for I2S DMA mode
    if(frame_id > 100){
      unsigned long pass_tm, curr_tm;
      curr_tm = millis();
      pass_tm = curr_tm - prev_tm0;
      prev_tm0 = curr_tm;
      Serial.printf("%d:max-%dms, total-%d\n", frame_id, max_pass_tm, pass_tm);
      frame_id = 0;
      max_pass_tm = 0;
    }
  }


}

