#ifndef DRSCRIPT_H
#define DRSCRIPT_H


#include "./parse_gen.h";
#include "./logger.h";
#include "./led_strip.h";
#include "./config.h";
#include "./standard.h";

namespace DevRelief {
    class Script {
        public:
            Script() {
                m_logger = new Logger("Script");
            }

            ~Script() { 
                delete m_logger;
            }

        private:
            Logger* m_logger;

    };
}
#endif