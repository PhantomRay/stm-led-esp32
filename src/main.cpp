#include <Arduino.h>
#include "SPIFFS.h"
#include "command_parsing.h"
#include "display.h"

void setup() {
  // put your setup code here, to run once:
  delay(2000);
  Serial.begin(115200);
  delay(100);

  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  display_init();

  xTaskCreate(&command_task, "command_task", 4096 /*configMINIMAL_STACK_SIZE*/, NULL, 5, NULL);
}

void loop() {
  /* display_task */
  display_task();
  // test_display_text();
}
