#line 1 "d:\\dev\\arduino\\led_app\\basic_controller\\lib\\script_executor.h"
#ifndef DRSCRIPTEXECUTOR_H
#define DRSCRIPTEXECUTOR_H


#include "./parse_gen.h";
#include "./logger.h";
#include "./led_strip.h";
#include "./config.h";
#include "./standard.h";
#include "./script.h";
namespace DevRelief {

    class ScriptExecutor {
        public:
            ScriptExecutor() {
                m_logger = new Logger("ScriptExecutor");
                m_script = NULL;
            }

            ~ScriptExecutor() { 
                delete m_logger;
                delete m_script;
            }
            void turnOff() {
                m_logger->error("turnOff not implemented");
            }

            void setScript(Script * script) {
                delete m_script;
                m_script = script;
            }

        private:
            Logger* m_logger;
            Script* m_script;
    };

}
#endif