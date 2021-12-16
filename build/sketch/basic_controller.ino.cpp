#include <Arduino.h>
#line 1 "d:\\dev\\arduino\\led_app\\basic_controller\\basic_controller.ino"
#include "./env.h"
#include "./lib/logger.h"
#include "./lib/basic_controller_app.h"
using namespace DevRelief;

Logger * logger;
Application * app;

#line 9 "d:\\dev\\arduino\\led_app\\basic_controller\\basic_controller.ino"
void setup();
#line 20 "d:\\dev\\arduino\\led_app\\basic_controller\\basic_controller.ino"
void loop();
#line 9 "d:\\dev\\arduino\\led_app\\basic_controller\\basic_controller.ino"
void setup() {
  // put your setup code here, to run once:
 logger = new Logger("LEDAPP");
  logger->info("creating app");
  app = new BasicControllerApplication();
  logger->info("created app");
  wdt_enable(WDTO_4S);
}

int count = 1;

void loop() {
  app->loop();
  wdt_reset();
}


