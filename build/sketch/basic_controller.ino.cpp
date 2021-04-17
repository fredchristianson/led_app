#include <Arduino.h>
#line 1 "d:\\dev\\arduino\\led_app\\basic_controller.ino"


#include "src/lib/logger.h"
#include "src/board/basic_controller_app.h"
using namespace DevRelief;

Logger * logger;
Application * app;

#line 10 "d:\\dev\\arduino\\led_app\\basic_controller.ino"
void setup();
#line 20 "d:\\dev\\arduino\\led_app\\basic_controller.ino"
void loop();
#line 10 "d:\\dev\\arduino\\led_app\\basic_controller.ino"
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

