#line 1 "d:\\dev\\arduino\\led_app\\basic_controller\\lib\\script\\script.h"
#ifndef DRSCRIPT_H
#define DRSCRIPT_H

#include "../parse_gen.h";
#include "../logger.h";
#include "../led_strip.h";
#include "../config.h";
#include "../standard.h";
#include "../list.h";
#include "../led_strip.h";
#include "./script_interface.h"
#include "./script_value.h"
#include "./script_position.h"
#include "./script_state.h"
#include "./script_command.h"
#include "./animation.h";

namespace DevRelief
{
 
    class Script : IScript
    {
    public:
        Script()
        {
            memLogger->debug("Script()");
            m_logger = &ScriptLogger;
            m_logger->debug("Create Script()");
            m_frequencyMSecs = 50;
            m_state = NULL;
        }

        virtual ~Script()
        {
            memLogger->debug("~Script()");
            m_logger->debug("~Script()");
            delete m_state;
        }

        void destroy() override { delete this;}

        void add(IScriptCommand *cmd)
        {
            if (cmd == NULL) {
                m_logger->error("Script.addCommand called with NULL cmd");
            }
            m_logger->info("Script.addCommand type %s",cmd->getType());
            m_commandList.add(cmd);
        }

        void begin(IHSLStrip * ledStrip) override {
            m_logger->debug("begin Script");
            delete m_state;
            m_state = new ScriptState();
            m_commandList.setStrip(ledStrip);
            m_state->beginScript(this,ledStrip);

        }

        void step() override
        {
            m_logger->test("step()");
            int ms = m_state->msecsSinceLastStep();
            m_logger->test("frequency %d %d",m_frequencyMSecs,ms);
            if (m_frequencyMSecs > m_state->msecsSinceLastStep())
            {
                m_logger->test("too soon to run");
                return;
            }
            m_logger->test("Script step");

            IHSLStrip * strip = m_state->getStrip();
            if (strip == NULL) {
                m_logger->test("Script needs a strip");
                return;
            }
            m_logger->test("clearStrip()");

            strip->clear();
            m_logger->test("beginStep()");
            m_state->beginStep();
            m_logger->test("execute()");
            m_commandList.execute(m_state);
            m_logger->test("endStep()");
            
            m_state->endStep();
            m_logger->test("show strip");
            strip->show();
        }

        void setName(const char *name) { m_name = name; }
        const char *getName() { return m_name.text(); }
        void setFrequencyMSec(int msecs) { m_frequencyMSecs = msecs; }
        int getFrequencyMSec() { return m_frequencyMSecs; }

    private:
        Logger *m_logger;
        ScriptCommandList m_commandList;
        PtrList<ScriptCommand *> m_commands;
        DRString m_name;
        int m_frequencyMSecs;
        ScriptState* m_state;
    };

   
}
#endif