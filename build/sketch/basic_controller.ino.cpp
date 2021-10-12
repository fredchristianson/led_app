#include <Arduino.h>
#line 1 "d:\\dev\\arduino\\led_app\\basic_controller\\basic_controller.ino"
#define STRIP1_LEDS 150
#define STRIP2_LEDS 123
#define LED_COUNT (STRIP1_LEDS+STRIP2_LEDS)
const char* HOSTNAME="LivingRoomLEDs";

#include "./lib/logger.h"
#include "./lib/basic_controller_app.h"
using namespace DevRelief;

Logger * logger;
Application * app;

#line 13 "d:\\dev\\arduino\\led_app\\basic_controller\\basic_controller.ino"
void setup();
#line 23 "d:\\dev\\arduino\\led_app\\basic_controller\\basic_controller.ino"
void loop();
#line 13 "d:\\dev\\arduino\\led_app\\basic_controller\\basic_controller.ino"
void setup() {
  // put your setup code here, to run once:
 logger = new Logger("droom");
  logger->info("creating app");
  app = new BasicControllerApplication();
  logger->info("created app");
}

int count = 1;

void loop() {
  app->loop();

}

