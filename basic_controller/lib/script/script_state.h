#ifndef DRSCRIPT__STATE_H
#define DRSCRIPT_STATE_H

#include "../parse_gen.h";
#include "../logger.h";
#include "../led_strip.h";
#include "../config.h";
#include "../standard.h";
#include "../list.h";
#include "../led_strip.h";
#include "./script_interface.h";
#include "./script_value.h";

namespace DevRelief
{
  

    class ScriptState
    {
    public:
        ScriptState()
        {
            memLogger->debug("create ScriptState");
            m_logger = &ScriptStateLogger;
            m_currentLedPosition = 0;
            m_stepNumber=0;
            m_lastStepTime = 0;
        }

        virtual ~ScriptState()
        {
            memLogger->debug("~ScriptState");
        }

       
      
        void beginStep()
        {
            long now = millis();
            m_lastStepTime = now;
            m_stepNumber++;
            m_currentLedPosition = 0;
            m_minLedPosition = 0;
            m_maxLedPosition = m_strip?m_strip->getCount()-1 : 0;
        }

        void endStep()
        {

        }

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

        void setLedPosition(int current,int min,int max) { 
            m_currentLedPosition = current;
            m_minLedPosition = min;
            m_maxLedPosition = max;
        }
        void setLedPosition(int pos) { m_currentLedPosition = pos;}
        int getLedPosition() { return m_currentLedPosition;}
        void setMinLedPosition(int pos) { m_minLedPosition = pos;}
        int getMinLedPosition() { return m_minLedPosition;}
        void setMaxLedPosition(int pos) { m_maxLedPosition = pos;}
        int getMaxLedPosition() { return m_maxLedPosition;}
        
    private:
        friend Script;
        void beginScript(Script *script, IHSLStrip *strip)
        {
            m_script = script;
            m_strip = strip;
            m_startTime = millis();
            m_lastStepTime = 0;
            m_stepNumber = 0;
            m_currentLedPosition = 0;
            m_minLedPosition = 0;
            m_maxLedPosition = 0;
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
        int m_currentLedPosition;
        int m_minLedPosition;
        int m_maxLedPosition;
    };

   
}
#endif