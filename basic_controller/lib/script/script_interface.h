#ifndef DRSCRIPT_INTERFACE_H
#define DRSCRIPT_INTERFACE_H

#include "../logger.h";
#include "../led_strip.h";
#include "../util.h";

namespace DevRelief
{
    Logger ScriptLogger("Script", SCRIPT_LOGGER_LEVEL);
    Logger ScriptMemoryLogger("ScriptMem", SCRIPT_MEMORY_LOGGER_LEVEL);
    Logger ScriptCommandLogger("ScriptCommand", SCRIPT_LOGGER_LEVEL);
    Logger ScriptStateLogger("ScriptState", SCRIPT_STATE_LOGGER_LEVEL);

    typedef enum PositionUnit
    {
        POS_PERCENT = 0,
        POS_PIXEL = 1,
        POS_INHERIT = 2
    };
    typedef enum PositionType
    {
        POS_RELATIVE = 0,
        POS_ABSOLUTE = 1,
        POS_AFTER = 3,
        POS_STRIP = 4
    };

    typedef enum ScriptStatus {
        SCRIPT_RUNNING,
        SCRIPT_COMPLETE,
        SCRIPT_ERROR,
        SCRIPT_PAUSED
    };

    class ScriptState;
    class Script;
    class ScriptCommand;
    class AnimationDomain;
    class AnimationRange;
    class TimeDomain;
    class PositionDomain;
    class IStripModifier;
    class IStripModifier;

    Logger *memLogger = &ScriptMemoryLogger;
    class IScriptCommand;

    class IScriptValue
    {
    public:
        virtual void destroy() =0; // cannot delete pure virtual interfaces. they must all implement destroy
        virtual int getIntValue(IScriptCommand* cmd,  int defaultValue) = 0;         // percent in [0,100]
        virtual double getFloatValue(IScriptCommand* cmd,  double defaultValue) = 0; // percent in [0,100]
        virtual bool getBoolValue(IScriptCommand* cmd,  bool defaultValue) = 0; // percent in [0,100]

        virtual bool isRecursing() = 0; // mainly for variable values
        // for debugging
        virtual DRString toString() = 0;
        virtual bool equals(IScriptCommand*cmd, const char * match)=0;

    };

    class IScriptValueProvider
    {
    public:
        virtual void destroy() =0; // cannot delete pure virtual interfaces. they must all implement destroy
        virtual bool hasValue(const char *name) = 0;
        virtual IScriptValue *getValue(const char *name) = 0;
    };




    class IScriptControl
    {
        // state: running, paused, stopped, waiting
        // "if": "equal:var(xyz),0"
        // frequency, duration, speed
        // m_variables (probably rand and time based)
        // m_threads - copies of "next chains" with concrete m_variables (copy state?  new state has separate start/run times)
        // leds off after duration
        public:
        virtual void destroy() =0; // cannot delete pure virtual interfaces. they must all implement destroy    
    };


    class IScriptState {
        public:
        virtual void destroy() =0; // cannot delete pure virtual interfaces. they must all implement destroy    
        virtual void setLedPosition(int pos, int min, int max)=0;

        virtual int getStepStartTime()=0;
        virtual TimeDomain* getAnimationTimeDomain()=0;
        virtual PositionDomain* getAnimationPositionDomain()=0;
        virtual int getStepNumber()=0;
    };
    
    class IScriptCommand 
    {
        public:
            virtual void destroy() =0; // cannot delete pure virtual interfaces. they must all implement destroy            
            virtual ScriptStatus execute(ScriptState*state)=0;
            virtual IScriptValue* getValue(const char* name)=0;
            virtual const char * getType()=0;
            virtual IHSLStrip * getHSLStrip()=0;
            virtual IStripModifier* getStrip()=0;
            virtual IScriptState* getState()=0;
            virtual IStripModifier* getPosition()=0;
    };

    class IValueAnimator
    {
        public:
        virtual void destroy() =0; // cannot delete pure virtual interfaces. they must all implement destroy    
        virtual bool isUnfolded(IScriptCommand *cmd)=0;
        virtual double get(IScriptCommand*cmd, AnimationRange&range)=0;
    };

  
    class IScript {
        public:
            virtual void destroy()=0;
            virtual void begin(IHSLStrip * ledStrip)=0;
            virtual void step()=0;
    };

    class IStripModifier : public IHSLStrip {
        public:
            virtual void destroy()=0;
            virtual PositionUnit getPositionUnit()=0;
            virtual int getEnd()=0;
            virtual int getOffset()=0;
    };

    class ICommandContainer  {
        public:
            virtual void add(IScriptCommand*cmd)=0;
            virtual IHSLStrip * getHSLStrip()=0;
            virtual IStripModifier* getStrip()=0;
    };
}
#endif