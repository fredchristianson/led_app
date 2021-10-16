#include <Arduino.h>
#line 1 "d:\\dev\\arduino\\led_app\\basic_controller\\basic_controller.ino"
#define STRIP1_LEDS 75
#define STRIP2_LEDS 75
#define STRIP3_LEDS 75
#define STRIP4_LEDS 75
#define LED_COUNT (STRIP1_LEDS+STRIP2_LEDS+STRIP3_LEDS+STRIP4_LEDS)


#include "./lib/logger.h"
#include "./lib/basic_controller_app.h"
using namespace DevRelief;

Logger * logger;
Application * app;

#line 15 "d:\\dev\\arduino\\led_app\\basic_controller\\basic_controller.ino"
void setup();
#line 26 "d:\\dev\\arduino\\led_app\\basic_controller\\basic_controller.ino"
void loop();
#line 15 "d:\\dev\\arduino\\led_app\\basic_controller\\basic_controller.ino"
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

