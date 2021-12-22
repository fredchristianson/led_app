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
#include "./script_container.h"
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
            m_rootContainer = new ScriptRootContainer();
        }

        virtual ~Script()
        {
            memLogger->debug("~Script()");
            m_logger->debug("~Script()");
            delete m_state;
            delete m_rootContainer;
        }

        void destroy() override { delete this;}


        void begin(IHSLStrip * ledStrip) override {
            m_logger->always("begin Script.  frequency %d",m_frequencyMSecs);
            delete m_state;
            m_state = new ScriptState();
            m_rootContainer->setStrip(ledStrip);
            m_state->beginScript(this,ledStrip);

        }

        void step() override
        {
            int ms = m_state->msecsSinceLastStep();
            m_logger->test("frequency %d %d",m_frequencyMSecs,ms);
            if (m_frequencyMSecs > m_state->msecsSinceLastStep())
            {
                 return;
            }
            m_logger->test("Script step");

            IHSLStrip * strip = m_state->getStrip();


            strip->clear();
            m_state->beginStep();
            m_rootContainer->execute(m_state);
            m_state->endStep();
            strip->show();
        }

        void setName(const char *name) { m_name = name; }
        const char *getName() { return m_name.text(); }
        void setFrequencyMSec(int msecs) { m_frequencyMSecs = msecs; }
        int getFrequencyMSec() { return m_frequencyMSecs; }
        ScriptRootContainer* getContainer() { return m_rootContainer;}
    private:
        Logger *m_logger;
        ScriptRootContainer* m_rootContainer;
        PtrList<ScriptCommand *> m_commands;
        DRString m_name;
        int m_frequencyMSecs;
        ScriptState* m_state;
    };

   
}
#endif