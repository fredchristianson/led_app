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
    class ChildState;

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
            m_currentContainer = NULL;
            m_strip = NULL;
        }

        virtual ~ScriptState()
        {
            memLogger->debug("~ScriptState");
        }

        void destroy() override { delete this;}
      
        void beginStep() override
        {
            long now = millis();
            m_lastStepTime = now;
            m_stepNumber++;
             m_previousCommand = NULL;
            m_currentCommand = NULL;

        }

        IScriptCommand* setContainer(IScriptCommand* container) {
            auto old = m_currentContainer;
            m_currentContainer = container;
            return old;
        }

        IHSLStrip* setStrip(IHSLStrip* strip) {
            auto old = m_strip;
            m_strip = strip;
            return old;
        }

        IScriptCommand* getContainer() { return m_currentContainer;}
        IHSLStrip* getStrip() { return m_strip;}

        void endStep() override
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
            m_logger->never("\tgot %x",val);
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

        void setValue(const char * valueName, IScriptValue* val) {
            m_values->addValue(valueName,val);
        }

        IScriptValue* getValue(void* owner,const char * valueName){
            DRFormattedString fullName("%04x-%s",owner,valueName);
            return getValue(fullName.get());
        }
        
        IScriptState* createChild();
    protected:
        friend Script;
        friend ChildState;
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

        Logger *m_logger;
        unsigned long m_startTime;
        unsigned long m_lastStepTime;
        Script *m_script;
        IScriptCommand* m_previousCommand;

        ScriptValueList *m_values;
        IScriptCommand* m_currentCommand;
        IHSLStrip * m_strip;
        IScriptCommand* m_currentContainer;
    };

    class ChildState : public ScriptState {
        public:
            ChildState(ScriptState* parent) {
                m_parent = parent;
                m_stepNumber = 0;
                m_logger = parent->m_logger;
                m_startTime = millis();
                m_strip = parent->getStrip();
                m_lastStepTime = 0;
                m_script = parent->m_script;
                m_previousCommand = NULL;
                m_currentCommand = NULL;
                m_currentContainer = parent->getContainer();
                m_logger->never("Created ChildState 0x%x 0x%x",m_strip,m_currentContainer);
            }

        protected:
            IScriptState* m_parent;
    };

   
    IScriptState*   ScriptState::createChild() {
        ChildState* child = new ChildState(this);
        return child;
    }
}
#endif