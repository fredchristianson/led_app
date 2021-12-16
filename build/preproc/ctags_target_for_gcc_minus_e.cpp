# 1 "d:\\dev\\arduino\\led_app\\basic_controller\\basic_controller.ino"
# 2 "d:\\dev\\arduino\\led_app\\basic_controller\\basic_controller.ino" 2
# 3 "d:\\dev\\arduino\\led_app\\basic_controller\\basic_controller.ino" 2
# 4 "d:\\dev\\arduino\\led_app\\basic_controller\\basic_controller.ino" 2
using namespace DevRelief;

Logger * logger;
Application * app;

void setup() {
  // put your setup code here, to run once:
 logger = new Logger("LEDAPP");
  logger->info("creating app");
  app = new BasicControllerApplication();
  logger->info("created app");
  ESP.wdtEnable(WDTO_4S);
}

int count = 1;

void loop() {
  app->loop();
  ESP.wdtFeed();
}
