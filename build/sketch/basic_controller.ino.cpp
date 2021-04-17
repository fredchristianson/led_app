#include <Arduino.h>
#line 1 "d:\\dev\\arduino\\led_app\\basic_controller.ino"
#include "src/lib/logger.h"
#include "src/board/basic_controller_app.h"
using namespace DevRelief;

Logger * logger;
Application * app;

#line 8 "d:\\dev\\arduino\\led_app\\basic_controller.ino"
void setup();
#line 18 "d:\\dev\\arduino\\led_app\\basic_controller.ino"
void loop();
#line 8 "d:\\dev\\arduino\\led_app\\basic_controller.ino"
void setup() {
  // put your setup code here, to run once:
 logger = new Logger("basic_controller");
  logger->info("creating app");
  app = new BasicControllerApplication();
  logger->info("created app");
}

int count = 1;

void loop() {
  app->loop();

}

