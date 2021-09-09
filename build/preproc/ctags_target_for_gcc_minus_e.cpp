# 1 "d:\\dev\\arduino\\led_app\\basic_controller\\basic_controller.ino"



const char* HOSTNAME="DiningRoomLEDs";

# 7 "d:\\dev\\arduino\\led_app\\basic_controller\\basic_controller.ino" 2
# 8 "d:\\dev\\arduino\\led_app\\basic_controller\\basic_controller.ino" 2
using namespace DevRelief;

Logger * logger;
Application * app;

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
