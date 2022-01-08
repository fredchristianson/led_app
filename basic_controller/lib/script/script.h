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
        }

        void destroy() override { delete this;}


        void begin(IHSLStrip * ledStrip) override {
            m_logger->never("begin Script.  frequency %d",m_frequencyMSecs);
            delete m_state;
            m_state = new ScriptState();
            m_logger->never("\tset strip 0x%04X",ledStrip);
            m_rootContainer->setStrip(ledStrip);
            m_logger->never("\tm_state->beginScript");
            m_state->beginScript(this,ledStrip);

        }

        void step() override
        {
            int ms = m_state->msecsSinceLastStep();
            m_logger->never("frequency %d %d",m_frequencyMSecs,ms);
            if (m_frequencyMSecs > m_state->msecsSinceLastStep())
            {   m_logger->never("frequency too soon");
                 return;
            }
            m_logger->never("Script step");

            IHSLStrip * strip = m_state->getStrip();
            m_logger->never("strip 0x%04X",strip);

            int startMs = millis();

            strip->clear();
            m_logger->never("cleared");
            m_state->beginStep();
            m_logger->never("began");
            m_logger->never("root container 0x%04X",m_rootContainer);
            m_rootContainer->execute(m_state);
            m_logger->never("executed");
            m_state->endStep();
            m_logger->never("ended");
            strip->show();
            m_logger->never("done %d",millis()-startMs);
        }

        void setName(const char *name) { m_name = name; }
        const char *getName() { return m_name.text(); }
        void setFrequencyMSec(int msecs) { m_frequencyMSecs = msecs; }
        int getFrequencyMSec() { return m_frequencyMSecs; }
        ScriptRootContainer* getContainer() { return m_rootContainer;}
    private:
        Logger *m_logger;
        ScriptRootContainer* m_rootContainer;
        PtrList<IScriptCommand *> m_commands;
        DRString m_name;
        int m_frequencyMSecs;
        ScriptState* m_state;
    };

   
}
#endif