#define STRIP1_LEDS 150
#define STRIP2_LEDS 123
#define LED_COUNT (STRIP1_LEDS+STRIP2_LEDS)
const char* HOSTNAME="LivingRoomLEDs";

#include "./lib/logger.h"
#include "./lib/basic_controller_app.h"
using namespace DevRelief;

Logger * logger;
Application * app;

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
