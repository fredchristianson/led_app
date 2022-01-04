#ifndef DRSCRIPT_STATE_H
#define DRSCRIPT_STATE_H

#include "../parse_gen.h"
#include "../logger.h"
#include "../led_strip.h"
#include "../config.h"
#include "../standard.h"
#include "../list.h"
#include "../led_strip.h"
#include "./script_interface.h"
#include "./animation.h"

namespace DevRelief
{
  

    class ScriptState : public IScriptState, IScriptValueProvider
    {
    public:
        ScriptState() 
        {
            memLogger->debug("create ScriptState");
            m_logger = &ScriptStateLogger;
            m_previousCommand = NULL;

            m_startTime = millis();
            m_lastStepTime = 0;
            m_stepNumber = 0;
            m_values = new ScriptValueList();
            
            m_currentCommand = NULL;
        }

        virtual ~ScriptState()
        {
            memLogger->debug("~ScriptState");
        }

        void destroy() override { delete this;}
      
        void beginStep()
        {
            long now = millis();
            m_lastStepTime = now;
            m_stepNumber++;
             m_previousCommand = NULL;
            m_currentCommand = NULL;

        }

        void endStep()
        {

        }

        int getStepStartTime() override { return m_lastStepTime;}
        long msecsSinceLastStep() { 
            long now = millis();
            return now - m_lastStepTime; 
        }
        long secondsSinceLastStep() {
            long now = millis();
            return (now - m_lastStepTime)/1000; 

         }

        IHSLStrip *getStrip() { return m_strip; }

        long scriptTimeMsecs() { return millis()-m_startTime;}

        int getStepNumber() override { return m_stepNumber;}
        IScriptCommand * getCurrentCommand() { return m_currentCommand;}
        void setCurrentCommand(IScriptCommand*cmd) { 
            m_logger->never("current command 0x%04X",cmd);
            m_currentCommand = cmd;
        }

        IScriptCommand * getPreviousCommand() { return m_previousCommand;}
        void setPreviousCommand(IScriptCommand*cmd) { 
            m_logger->never("Previous command 0x%04X",cmd);
            m_previousCommand = cmd;
        }

                /* IScriptValueProvider */
        bool hasValue(const char *name) override
        {
            return m_values == NULL ? false : m_values->hasValue(name) != NULL;
        }

        IScriptValue *getValue(const char *name) override
        {
            m_logger->never("getvalue %s",name);

            IScriptValue* val =  m_values == NULL ? NULL : m_values->getValue(name);
            return val;
        }

        int getIntValue(const char * name,int defaultValue) {
            IScriptValue* sv = getValue(name);
            if (sv == NULL) { return defaultValue;}
            return sv->getIntValue(m_currentCommand,defaultValue);
        }

        void setValue(void*owner, const char * valueName, IScriptValue* val) {
            DRFormattedString fullName("%04x-%s",owner,valueName);
            m_values->addValue(fullName.get(),val);
        }

        IScriptValue* getValue(void* owner,const char * valueName){
            DRFormattedString fullName("%04x-%s",owner,valueName);
            return getValue(fullName.get());
        }
        
    private:
        friend Script;
        void beginScript(Script *script, IHSLStrip *strip)
        {
            m_script = script;
            m_strip = strip;
            m_startTime = millis();
            m_lastStepTime = 0;
            m_stepNumber = 0;
            m_currentCommand = NULL;
        }

        void endScript(Script *script)
        {
        }


        int m_stepNumber;
        IHSLStrip *m_strip;

        Logger *m_logger;
        unsigned long m_startTime;
        unsigned long m_lastStepTime;
        Script *m_script;
        IScriptCommand* m_previousCommand;

        ScriptValueList *m_values;
        IScriptCommand* m_currentCommand;
    };

   
}
#endif