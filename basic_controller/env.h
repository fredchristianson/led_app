#define PROD 1
#define DEV 2
#define ENV DEV

#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__
#define BUILD_VERSION "2.0.1"
#define HOSTNAME "led_app_unset"

#define GENERATOR_LOGGER_LEVEL WARN_LEVEL
#define PARSER_LOGGER_LEVEL WARN_LEVEL
#define APP_LOGGER_LEVEL INFO_LEVEL
#define DATA_LOADER_LOGGER_LEVEL INFO_LEVEL
#define DATA_LOGGER_LEVEL INFO_LEVEL

#if ENV==PROD
    #define ENV_PROD
#else
    #define ENV_DEV
    #define DEBUG
    // RUN_TESTS should be 1 to run tests on start.  otherwise they are not run
    #define RUN_TESTS 0
#endif
