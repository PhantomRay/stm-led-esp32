#include <Arduino.h>
#include "SPIFFS.h"
#include "command_parsing.h"
#include "display.h"

void setup() {
  // put your setup code here, to run once:
  delay(1000);
  Serial.begin(115200);
  delay(100);

  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  File file    = SPIFFS.open("/exists");
  bool no_file = !(file.size() > 0); // DO NOT use !file
  file.close();

  if (no_file) {
    Serial.println("Failed to open file!");
  }

  display_init(no_file);

  // xTaskCreate(command_task, "command_task", 4096 /*configMINIMAL_STACK_SIZE*/, NULL, 5, NULL);
  xTaskCreatePinnedToCore(command_task,   /* Function to implement the task */
                          "command_task", /* Name of the task */
                          4096,           /* Stack size in words */
                          NULL,           /* Task input parameter */
                          0,              /* Priority of the task */
                          NULL,           /* Task handle. */
                          1);             /* Core where the task should run */

  Serial.println("setup() complete");
}

void loop() {
  /* display_task */
  display_task();
}
