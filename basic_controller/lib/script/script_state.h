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
  

    class ScriptState : public IScriptState
    {
    public:
        ScriptState() 
        {
            memLogger->debug("create ScriptState");
            m_logger = &ScriptStateLogger;
            m_stepNumber=0;
            m_lastStepTime = 0;
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
            m_positionDomain.setPosition(0,0,0);
            m_timeDomain.setTimePosition(now);
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

        void setLedPosition(int current,int min,int max) { 
            m_positionDomain.setPosition(current,min,max);
        }

        TimeDomain* getAnimationTimeDomain() {
            return &m_timeDomain;
        }        

        PositionDomain* getAnimationPositionDomain(){
            return &m_positionDomain;
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
            m_positionDomain.setPosition(0,0,0);
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

        TimeDomain m_timeDomain;
        PositionDomain m_positionDomain;
    };

   
}
#endif