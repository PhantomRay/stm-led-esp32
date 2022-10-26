#include <Arduino.h>
#include "SPIFFS.h"
#include "command_parsing.h"
#include "display.h"

uint32_t frame_id = 0;

void setup() {
  // put your setup code here, to run once:
  delay(2000);
  Serial.begin(115200);
  delay(100);
  Serial.println("I2S DMA.");

  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  display_init();

  xTaskCreate(&command_task, "command_task", 4096 /*configMINIMAL_STACK_SIZE*/, NULL, 5, NULL);
}

/* current log result: pass_tm - 24 or 25ms = (10 or 11) + 14*/
void loop() {
  /* display_task */
  if (command_desc_update_flag) {
    display_command          = command_desc[current_display_description_id];
    command_desc_update_flag = false;
  }
  update_display_param(display_command);
  vTaskDelay(200);
}
