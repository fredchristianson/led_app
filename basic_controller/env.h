#ifndef ENV_H
#define ENV_H

#define PROD 1
#define DEV 2
#define ENV DEV

#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__
#define BUILD_VERSION "2.0.1"
#define HOSTNAME "led_app_unset"

// LOGGING_ON should be 1 to enable logging.  0 optimizes all logging calls and constants (messages) out.

#define LOGGING_ON 1
#define ADAFRUIT_LED_LOGGER_LEVEL INFO_LEVEL
#define ANIMATION_LOGGER_LEVEL DEBUG_LEVEL
#define APP_LOGGER_LEVEL DEBUG_LEVEL
#define COMPOUND_STRIP_LOGGER_LEVEL INFO_LEVEL
#define CONFIG_LOGGER_LEVEL INFO_LEVEL
#define DATA_LOADER_LOGGER_LEVEL DEBUG_LEVEL
#define DATA_LOGGER_LEVEL INFO_LEVEL
#define DRSTRING_LOGGER_LEVEL WARN_LEVEL
#define ENSURE_LOGGER_LEVEL DEBUG_LEVEL
#define GENERATOR_LOGGER_LEVEL WARN_LEVEL
#define HSL_STRIP_LOGGER_LEVEL INFO_LEVEL
#define HTTP_SERVER_LOGGER_LEVEL INFO_LEVEL
#define LED_LOGGER_LEVEL INFO_LEVEL
#define LINKED_LIST_LOGGER_LEVEL INFO_LEVEL
#define PARSER_LOGGER_LEVEL WARN_LEVEL
#define PTR_LIST_LOGGER_LEVEL WARN_LEVEL
#define SCRIPT_EXECUTOR_LOGGER_LEVEL DEBUG_LEVEL
#define SCRIPT_LOADER_LOGGER_LEVEL WARN_LEVEL
#define SCRIPT_LOGGER_LEVEL DEBUG_LEVEL
#define SCRIPT_STATE_LOGGER_LEVEL DEBUG_LEVEL
#define SCRIPT_MEMORY_LOGGER_LEVEL WARN_LEVEL
#define TEST_LOGGER_LEVEL INFO_LEVEL

#if ENV==PROD
    #define ENV_PROD
#else
    #define ENV_DEV
    #define DEBUG
    #define ENSURE 1
    // RUN_TESTS should be 1 to run tests on start.  otherwise they are not run
    #define RUN_TESTS 0
    #define RUN_STRING_TESTS 0
    #define RUN_JSON_TESTS 1
#endif

#endif