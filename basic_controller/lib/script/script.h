#ifndef DRSCRIPT_H
#define DRSCRIPT_H

#include "../parse_gen.h";
#include "../logger.h";
#include "../led_strip.h";
#include "../config.h";
#include "../standard.h";
#include "../list.h";
#include "../led_strip.h";
#include "./animation.h";
#include "./json_names.h";

namespace DevRelief
{
    typedef enum PositionUnit
    {
        POS_PERCENT = 0,
        POS_PIXEL = 1
    };
    typedef enum PositionType
    {
        POS_RELATIVE = 0,
        POS_ABSOLUTE = 1,
        POS_AFTER = 3,
        POS_STRIP = 3
    };

    class ScriptState;
    class Script;
    class ScriptCommand;

    Logger ScriptLogger("Script", SCRIPT_LOGGER_LEVEL);
    Logger ScriptMemoryLogger("ScriptMem", SCRIPT_MEMORY_LOGGER_LEVEL);
    Logger ScriptCommandLogger("ScriptCommand", SCRIPT_LOGGER_LEVEL);
    Logger ScriptStateLogger("ScriptCommand", SCRIPT_STATE_LOGGER_LEVEL);

    Logger *memLogger = &ScriptMemoryLogger;

    class IScriptValue
    {
    public:
        
        virtual int getIntValue(ScriptState &state, double rangePositionPercent, int defaultValue) = 0;         // percent in [0,100]
        virtual double getFloatValue(ScriptState &state, double rangePositionPercent, double defaultValue) = 0; // percent in [0,100]
        virtual bool getBoolValue(ScriptState &state, double rangePositionPercent, bool defaultValue) = 0; // percent in [0,100]

        // for debugging
        virtual DRString toString() = 0;
    };

    class IScriptValueProvider
    {
    public:
        virtual bool hasValue(const char *name) = 0;
        virtual IScriptValue *getValue(const char *name) = 0;
    };


    class IScriptPosition : public IHSLStrip
    {
    public:
        virtual ~IScriptPosition() {}
        virtual IHSLStrip *getPrevious() = 0;
        virtual void updateValues(ScriptState &state, IHSLStrip *) = 0;
        // start, count, step, wrap?, reverse, after (pixel after previous pos)
        // base: Relative (default), absolute, strip, after
        // IHSLStrip
        // wrap - index wraps at end
        // repeat - when start reaches end, go back to start
        // delay - time to wait before repeat
    };

    class IControl
    {
        // state: running, paused, stopped, waiting
        // "if": "equal:var(xyz),0"
        // frequency, duration, speed
        // m_variables (probably rand and time based)
        // m_threads - copies of "next chains" with concrete m_variables (copy state?  new state has separate start/run times)
        // leds off after duration
    };

    class IScriptCommand
    {
    };

    class IValueAnimator
    {
    };

    class ScriptState
    {
    public:
        ScriptState()
        {
            memLogger->debug("create ScriptState");
            m_logger = &ScriptStateLogger;
            m_script = NULL;
            m_strip = NULL;
            m_currentCommand = NULL;
            m_previousCommand = NULL;
            m_startTime = 0;
            m_lastStepTime = 0;
            m_position = NULL;
        }

        virtual ~ScriptState()
        {
            memLogger->debug("~ScriptState");
        }

        void eachLed(auto &&lambda);

        void addValueProvider(IScriptValueProvider *provider)
        {
            m_logger->test("add valueProvider 0x%04X",provider);
            m_valueProviders.insertAt(0, provider);
            m_logger->test("\tsize(): %d",m_valueProviders.size());
        }

        IScriptValue* findValue(const char * name,auto lambda) {
            IScriptValue * value=NULL;
            m_logger->test("find value %s from %d providers",name,m_valueProviders.size());
            IScriptValueProvider **providerPtr = m_valueProviders.first([&](IScriptValueProvider *p) {
                m_logger->test("\tcheck provider 0x%04X",p);
                IScriptValue* v = p->getValue(name);
                if (v != NULL && m_lookupPath.firstIndexOf(v)<0) {
                    m_logger->test("\tprovider is good");
                    value = v;
                    return true;
                } else {
                    m_logger->test("\tIScriptValue ",(v ? "has already been used - avoid loop ": "is good"));
                }
                return false; 
            });

            if (value == NULL)
            {
                m_logger->test("ScriptValueProvider does not have value named %s", name);
            }
            else
            {
                m_logger->test("\tadd value to lookup path 0x%04X %s",*providerPtr,name);
                m_lookupPath.add(value);
                lambda(value);
                m_lookupPath.removeFirst(value);
            }
            return value;
        }

        int getIntValue(const char *name, double rangePositionPercent, int defaultValue)
        {
            m_logger->test("getIntValue for %s",name);
            int result = defaultValue;
            IScriptValue *value = findValue(name,[&](IScriptValue*value){
                m_logger->test("found variable");
                result = value->getIntValue(*this,rangePositionPercent,defaultValue);
            });
            return result;
        }



        double getFloatValue(const char *name, double rangePositionPercent, double defaultValue)
        {
            m_logger->never("getIntValue for %s",name);
            double result = defaultValue;
            IScriptValue *value = findValue(name,[&](IScriptValue*value){
                result = value->getFloatValue(*this,rangePositionPercent,defaultValue);
            });
            return result;
        }

        double getBoolValue(const char *name, double rangePositionPercent, bool defaultValue)
        {
            m_logger->never("getIntValue for %s",name);
            bool result = defaultValue;
            IScriptValue *value = findValue(name,[&](IScriptValue*value){
                result = value->getBoolValue(*this,rangePositionPercent,defaultValue);
            });
            return result;
        }

        void beginScript(Script *script, IHSLStrip *strip)
        {
            m_script = script;
            m_strip = strip;
            m_startTime = millis();
            m_lastStepTime = 0;
            m_stepNumber = 0;
        }

        void endScript(Script *script)
        {
        }

        void beginStep()
        {
            m_position = NULL;
            m_currentCommand = NULL;
            m_previousCommand = NULL;
            m_previousCommands.clear();
            m_valueProviders.clear();
            long now = millis();
            m_lastStepTime = now;
            m_stepNumber++;
        }

        void endStep()
        {
            m_currentCommand = NULL;
            m_previousCommand = NULL;
            m_previousCommands.clear();
        }

        void beginCommand(IScriptCommand *cmd)
        {
            m_currentCommand = cmd;
        }

        void endCommand(IScriptCommand *cmd)
        {
            m_previousCommand = m_currentCommand;
            m_currentCommand = NULL;
            m_previousCommands.insertAt(0, cmd);
        }

        long msecsSinceLastStep() { 
            long now = millis();
            return now - m_lastStepTime; 
        }
        long secondsSinceLastStep() {
            long now = millis();
            return (now - m_lastStepTime)/1000; 

         }

        void addPosition(IScriptPosition *position)
        {
            m_logger->test("Add script position 0x%04X",position);
                m_logger->test("\tprevious position 0x%04x",m_position);
            if (m_position != NULL)
            {
                position->updateValues(*this, m_position);
            }
            else
            {
                m_logger->never("\tset previous to strip");
                position->updateValues(*this, m_strip);
            }
            m_position = position;
        }

        void removePosition(IScriptPosition *position)
        {
            m_logger->test("Remove script position");   
            if (position != m_position)
            {
                m_logger->error("can only remove last position");
                return;
            }
            IHSLStrip *prev = position->getPrevious();
            if (prev == m_strip)
            {
                m_position = NULL;
            }
            else
            {
                m_position = (IScriptPosition *)prev;
            }
        }

        IHSLStrip *getStrip() { return m_position != NULL ? m_position : m_strip; }
        IHSLStrip *getBaseStrip() { return m_strip; }

        long scriptTimeMsecs() { return millis()-m_startTime;}
    private:
        int m_stepNumber;
        IHSLStrip *m_strip;
        IScriptPosition *m_position;

        LinkedList<IScriptPosition *> m_previousPositions;
        LinkedList<IScriptCommand *> m_previousCommands;
        LinkedList<IScriptValueProvider *> m_valueProviders;
        LinkedList<IScriptValue *> m_lookupPath;
        // vars: time (ms/s/?), step
        Logger *m_logger;
        IScriptCommand *m_currentCommand;
        IScriptCommand *m_previousCommand;
        unsigned long m_startTime;
        unsigned long m_lastStepTime;
        Script *m_script;
    };

    class ValueAnimator : public IValueAnimator
    {
        public:
            ValueAnimator() {
                m_logger=&ScriptLogger;
                m_speedValue=0;
                m_durationValue=0;
                m_repeatValue=0;
                m_repeatDelayValue=0;
                m_unfoldValue = NULL;
            }

            virtual ~ValueAnimator() {
                delete m_speedValue;
                delete m_durationValue;
                delete m_repeatValue;
                delete m_repeatDelayValue;
                delete m_unfoldValue;
            };

            void setSpeed(IScriptValue* speed) { delete m_speedValue ; m_speedValue = speed;}
            void setDuration(IScriptValue* duration) { delete m_durationValue ; m_durationValue = duration;}
            void setRepeat(IScriptValue* repeat) { delete m_repeatValue ; m_repeatValue = repeat;}
            void setRepeatDelay(IScriptValue* repeatDelay) { delete m_repeatDelayValue ; m_repeatDelayValue = repeatDelay;}
            

            double getTimeValue(ScriptState& state, double low, double high) {
                m_logger->test("ValueAnimator::getTimeValue %f %f",low,high);
                double stepsPerSecond = 0;
                if (low == high) {
                    m_logger->test("\tvalues equal");
                    return low;
                }
                double range = high-low+1;
                int durationMsecs = 0;
                if (m_speedValue != NULL) {
                    double speed = m_speedValue->getFloatValue(state,0,0);
                    stepsPerSecond = speed;
                    durationMsecs =  (int)((range/stepsPerSecond)*1000);
                } else if (m_durationValue != NULL) {
                    durationMsecs = m_durationValue->getFloatValue(state,0,0);
                    if (durationMsecs != 0) {
                        int steps = high-low+1;
                        stepsPerSecond = 1000*steps/durationMsecs;
                    }
                }
                m_logger->test("\tspeed=%f. duration=%d",stepsPerSecond,durationMsecs);
                if (durationMsecs == 0) {
                    return low;
                }
                int time = state.scriptTimeMsecs();
                double timePosition = time % durationMsecs;
                m_logger->test("\ttime=%d. timeposition=%f",time,timePosition);

                bool unfold = false;
                if (m_unfoldValue){
                    unfold = m_unfoldValue->getBoolValue(state,0,false);
                }
                Animator animator(0,durationMsecs, unfold);
                double value = animator.getValueAt(low,high,timePosition);
                m_logger->debug("\ttime animator value %f",value);
                return value;
            }

            double getPositionValue(ScriptState&state,double low, double high,double percent) {
                m_logger->debug("getPositionValue %f %f %f",low,high,percent);
                Animator animate(low,high,isUnfolded(state));
                double val = animate.getValueAtPercent(low,high,percent);
                m_logger->debug("\tposition animator value %f",val);
                return val;
            }

            void setUnfold(IScriptValue* unfold) {
                delete m_unfoldValue;
                m_unfoldValue = unfold;
            }

            bool isUnfolded(ScriptState& state) {
                return m_unfoldValue && m_unfoldValue->getBoolValue(state,0,false);
            }

            bool hasSpeedOrDuration(ScriptState&state) {
                return (m_speedValue && m_speedValue->getIntValue(state,0,0)>0)||
                        (m_durationValue && m_durationValue->getIntValue(state,0,0)>0);
            }

            bool canAnimateOverTime(ScriptState&state) { 
                m_logger->test("canAnimateOverTime");
                bool speedOrDuration = hasSpeedOrDuration(state);
                m_logger->test("\t%s",speedOrDuration?"yes":"no");
                return speedOrDuration;
            }
        protected:
            IScriptValue* m_speedValue; 
            IScriptValue* m_durationValue;  // ignored if m_seed is set
            IScriptValue*    m_repeatValue;
            IScriptValue*    m_repeatDelayValue;
            IScriptValue* m_unfoldValue;
            Logger* m_logger;
    };

    class TimeValueAnimator : public ValueAnimator
    {
        // wrap - index wraps at end
        // repeat - when start reaches end, go back to start
        // delay - time to wait before repeat
        // unfold
    };

    class PositionAnimator : public ValueAnimator
    {
        public:
            PositionAnimator() {}
            ~PositionAnimator() {}
    };

    class ScriptPosition : public IScriptPosition
    {
    public:
        ScriptPosition()
        {
            ScriptLogger.debug("create ScriptPosition()");
            m_startValue = NULL;
            m_endValue = NULL;
            m_countValue = NULL;
            m_skipValue = NULL;
            m_unit = POS_PERCENT;
            m_type = POS_RELATIVE;
            m_previous = NULL;
            m_wrapValue = NULL;
            m_reverseValue = NULL;
            m_start = 0;
            m_count = 0;
            m_wrap = true;
            m_reverse = false;
            m_logger = &ScriptLogger;
            m_animation = NULL;
            m_animationOffset = 0;
        }

        ~ScriptPosition() {
            delete m_startValue;
            delete m_countValue;
            delete m_endValue;
            delete m_skipValue;
            delete m_wrapValue;
            delete m_reverseValue;
            delete m_animation;
        }

        void updateValues(ScriptState &state, IHSLStrip *strip);
        IHSLStrip *getPrevious()
        {
            return m_previous;
        }

        void setStartValue(IScriptValue *val) { m_startValue = val; }
        void setCountValue(IScriptValue *val) { m_countValue = val; }
        void setEndValue(IScriptValue *val) { m_endValue = val; }
        void setSkipValue(IScriptValue *val) {
            if (val != NULL) {
                delete m_skipValue;
                m_logger->debug("got skip value %s",val->toString().get());
                m_skipValue = val; 
            }
        }
        void setUnit(PositionUnit unit) { m_unit = unit; }
        void setHue(int index, int16_t hue, HSLOperation op = REPLACE)
        {
            if (m_previous == NULL || !translate(index)) {
                //ScriptLogger.periodic(ERROR_LEVEL,100,NULL,"Script position missing a previous strip");
                return;
            }
            m_previous->setHue((index), hue, op);
        }
        void setSaturation(int index, int16_t saturation, HSLOperation op = REPLACE)
        {
            if (m_previous == NULL || !translate(index)) {
                //ScriptLogger.periodic(ERROR_LEVEL,100,NULL,"Script position missing a previous strip");
                return;
            }
            m_previous->setSaturation((index), saturation, op);
        }
        void setLightness(int index, int16_t lightness, HSLOperation op = REPLACE)
        {
            if (m_previous == NULL || !translate(index)) {
                //ScriptLogger.periodic(ERROR_LEVEL,100,NULL,"Script position missing a previous strip");
                return;
            }
            m_previous->setLightness((index), lightness, op);
        }
        void setRGB(int index, const CRGB &rgb, HSLOperation op = REPLACE)
        {
            int orig = index;
            if (m_previous == NULL || !translate(index)) {
                //ScriptLogger.periodic(ERROR_LEVEL,1000,NULL,"Script position missing a previous strip");
                return;
            }
            m_logger->never("\ttranslated RGB index %d===>%d",orig,index);
            m_previous->setRGB((index), rgb, op);
        }
        size_t getCount() { return m_count; }
        size_t getStart() { return m_start; }
        bool translate(int& index)
        {
            m_logger->test("translate %d.  start=%d. count=%d. end=%d. skip=%d this=0x%04X anim:%d",index,m_start,m_count,m_end,m_skip,this,m_animationOffset);
            if (m_count == 0) { return false;}
            index = index + m_animationOffset;
            if (m_skipValue) {
                m_logger->test("\tskip %d. index=%d",m_skip,index*m_skip);
                index = index * m_skip;
            } else {
                m_logger->test("\tno skip");
            }
            if (index < 0) {
                if (m_wrap) {
                    index = m_count + (index%m_count);
                } else {
                    m_logger->test("clip");
                    return false;
                }
            }
            if (index >= m_count) {
                if (m_wrap) {
                    index = index%m_count;
                } else {
                    m_logger->test("clip");
                    return false;
                }
            }

            // always works in PIXEL units.  updateValues took care of % to pixel if needed

            if (m_start>m_end) {
                index = -index;

            }
            index = m_start+index;
            m_logger->test("\ttranslated %d",index);

            return true;
        }

        void setWrap(IScriptValue* wrap) { m_wrapValue = wrap; }
        void setReverse(IScriptValue* reverse) { m_reverseValue = reverse; }
        void clear() { m_previous->clear();}
        void show() { m_previous->show();}
        void setAnimator(PositionAnimator* animator) {
            m_logger->debug("delete old animator 0x%04x",m_animation);
            delete m_animation;
            m_logger->debug("set new animator");
            m_animation = animator;
            m_logger->debug("PositionAnimator 0x%04X",animator);
        }
    private:
        // values that may be variables
        IScriptValue* m_wrapValue;
        IScriptValue* m_reverseValue;
        IScriptValue *m_startValue;
        IScriptValue *m_endValue;
        IScriptValue *m_countValue;
        IScriptValue *m_skipValue;

        PositionAnimator* m_animation;
        int m_animationOffset;
        // evaluated variable values
        int m_start;
        int m_count;
        int m_end;
        bool m_wrap;
        bool m_reverse;
        int m_skip;
        PositionUnit m_unit;
        PositionType m_type;
        IHSLStrip *m_previous; // in ScriptCommand order
        IHSLStrip *m_base;      // may be different from previous for ABSOLUTE and other
        Logger* m_logger;
    };

    class ScriptValue : public IScriptValue {
        public:
            ScriptValue() {
                m_logger = &ScriptLogger;
            }
            virtual ~ScriptValue() {}
        protected:
            Logger* m_logger;
    };

    class ScriptVariableValue : public ScriptValue
    {
    public:
        ScriptVariableValue(const char *value, double defaultValue) : m_name(value), m_defaultValue(defaultValue)
        {
            memLogger->debug("ScriptVariableValue()");
            m_logger = &ScriptLogger;
            m_logger->debug("Created ScriptVariableValue %s.  default=%f", value, defaultValue);
        }

        virtual ~ScriptVariableValue()
        {
            memLogger->debug("~ScriptVariableValue");
        }
        virtual int getIntValue(ScriptState &state, double percent, int defaultValue) override
        {
            int val = state.getIntValue(m_name.text(), percent, m_defaultValue);
            m_logger->never("ScriptVariableValue %s got %d", m_name.text(), val);
            return val;
        }

        virtual double getFloatValue(ScriptState &state, double percent, double defaultValue) override
        {
            double val = state.getFloatValue(m_name.text(), percent, m_defaultValue);
            return val;
        }

        virtual bool getBoolValue(ScriptState &state, double percent, bool defaultValue) override
        {
            double val = state.getBoolValue(m_name.text(), percent, m_defaultValue);
            return val;
        }

        virtual DRString toString() { return DRString("Variable: ").append(m_name); }

    protected:
        DRString m_name;
        double m_defaultValue;
        Logger *m_logger;
    };

    class ScriptControl : public IControl
    {
    };

    class ScriptControlCommand : public ScriptControl
    {
    };


    class NameValue
    {
    public:
        NameValue(const char *name, ScriptValue *value)
        {
            ScriptMemoryLogger.debug("NameValue %s", name);
            m_name = name;
            m_value = value;
        }

        virtual ~NameValue()
        {
            ScriptMemoryLogger.debug("~NameValue() %s 0x%04X", m_name.text(),m_value);
            delete m_value;
        }

        const char *getName() { return m_name.text(); }
        ScriptValue *getValue() { return m_value; }

    private:
        DRString m_name;
        ScriptValue *m_value;
    };
    // ScriptVariableGenerator: ??? rand, trig, ...

    class ScriptFunctionValue : public ScriptValue
    {
        // todo: named functions, call?
    public:
        ScriptFunctionValue(const char *value) : m_value(value)
        {
            memLogger->debug("ScriptVariableValue()");
        }

        virtual ~ScriptFunctionValue()
        {
            memLogger->debug("~ScriptFunctionValue()");
        }
        int getIntValue(ScriptState &state, double percent, int defaultValue) override
        {
            return defaultValue;
        }

        double getFloatValue(ScriptState &state, double percent, double defaultValue) override
        {
            return defaultValue;
        }

       bool getBoolValue(ScriptState &state, double percent, bool defaultValue) override
        {
            return defaultValue;
        }

        DRString toString() { return DRString("Function: ").append(m_value); }

    protected:
        DRString m_value;
    };

    class ScriptNumberValue : public ScriptValue
    {
    public:
        ScriptNumberValue(double value) : m_value(value)
        {
            memLogger->debug("ScriptNumberValue()");
        }

        virtual ~ScriptNumberValue()
        {
            memLogger->debug("~ScriptNumberValue()");
        }

        virtual int getIntValue(ScriptState &state, double percent, int defaultValue) override
        {
            int v = m_value;
            m_logger->test("getIntValue %f %d",m_value,v);
            return v;
        }

        virtual double getFloatValue(ScriptState &state, double percent, double defaultValue) override
        {
            return m_value;
        }

        virtual bool getBoolValue(ScriptState &state, double percent, bool defaultValue) override
        {
            return m_value != 0;
        }

        virtual DRString toString() { return DRString::fromFloat(m_value); }

    protected:
        double m_value;
    };

    class ScriptBoolValue : public ScriptValue
    {
    public:
        ScriptBoolValue(bool value) : m_value(value)
        {
            memLogger->debug("ScriptBoolValue()");
            m_logger->debug("ScriptBoolValue()");
        }

        virtual ~ScriptBoolValue()
        {
            m_logger->debug("~ScriptBoolValue()");
            memLogger->debug("~ScriptBoolValue()");
        }

        int getIntValue(ScriptState &state, double percent, int defaultValue) override
        {
            return m_value ? 1 : 0;
        }

        double getFloatValue(ScriptState &state, double percent, double defaultValue) override
        {
            return m_value ? 1 : 0;
        }

        bool getBoolValue(ScriptState & state, double percent, bool defaultValue) override {
            return m_value;
        }

        DRString toString() override { 
            m_logger->debug("ScriptBoolValue.toString()");
            const char * val =  m_value ? "true":"false"; 
            m_logger->debug("\tval=%s",val);
            DRString drv(val);
            m_logger->debug("\tcreated DRString");
            return drv;
        }

    protected:
        bool m_value;
    };


    class ScriptStringValue : public ScriptValue
    {
    public:
        ScriptStringValue(const char *value) : m_value(value)
        {
            memLogger->debug("ScriptStringValue()");
        }

        virtual ~ScriptStringValue()
        {
            memLogger->debug("~ScriptStringValue()");
        }

        int getIntValue(ScriptState &state, double percent, int defaultValue) override
        {
            const char *n = m_value.text();
            if (n != NULL)
            {
                return atoi(n);
            }
            return defaultValue;
        }

        double getFloatValue(ScriptState &state, double percent, double defaultValue) override
        {
            const char *n = m_value.text();
            if (n != NULL)
            {
                return atof(n);
            }
            return defaultValue;
        }

        bool getBoolValue(ScriptState &state, double percent, bool defaultValue) override
        {
            const char *n = m_value.text();
            if (n != NULL)
            {
                return Util::equal(n,"true");
            }
            return defaultValue;
        }

        DRString toString() override { return m_value; }

    protected:
        DRString m_value;
    };

    class ScriptRangeValue : public ScriptValue
    {
    public:
        ScriptRangeValue(ScriptValue *start, ScriptValue *end)
        {
            memLogger->debug("ScriptRangeValue()");
            m_start = start;
            m_end = end;
            m_animate = NULL;
        }

        virtual ~ScriptRangeValue()
        {
            memLogger->debug("~ScriptRangeValue() start");
            delete m_start;
            delete m_end;
            delete m_animate;
            memLogger->debug("~ScriptRangeValue() end");
        }

        virtual int getIntValue(ScriptState &state, double percent, int defaultValue)
        {
            return (int)getFloatValue(state,percent,(double)defaultValue);
        }

        virtual double getFloatValue(ScriptState &state, double percent, double defaultValue)
        {
            if (m_start == NULL)
            {
                return m_end ? m_end->getIntValue(state, 100, defaultValue) : defaultValue;
            }
            else if (m_end == NULL)
            {
                return m_start ? m_start->getIntValue(state, 0, defaultValue) : defaultValue;
            }
            double start = m_start->getIntValue(state, percent, 0);
            double end = m_end->getIntValue(state, percent, 1);
            m_logger->test("check canAnimateOverTime");
            if (m_animate == NULL) {
                int diff = end - start;
                int result = start + diff * percent;
                return result;
            } else {
                if (m_animate->canAnimateOverTime(state)){
                    m_logger->test("animator has speed (animate over time");
                    return m_animate->getTimeValue(state,start,end);
                } else {
                    m_logger->test("\tuse position 0x%04X",m_animate);
                    return m_animate->getPositionValue(state,start,end,percent);

                }
            }
        }
        virtual bool getBoolValue(ScriptState &state, double percent, bool defaultValue)
        {
            int start = m_start->getIntValue(state, percent, 0);
            int end = m_end->getIntValue(state, percent, start);
            // bool probably doesn't make sense for a range.  return true if there is a range rather than single value
           return start != end;
        }

        virtual DRString toString()
        {
            DRString result("range:");
            result.append(m_start ? m_start->toString() : "NULL")
                .append(m_end ? m_end->toString() : "NULL");
        }

        void setAnimator(ValueAnimator* animator) {
            m_animate = animator;
        }
    protected:
        ScriptValue *m_start;
        ScriptValue *m_end;
        ValueAnimator* m_animate;
    };

    class ScriptPatternElement
    {
    public:
        ScriptPatternElement()
        {
            memLogger->debug("ScriptPatternElement()");
        }
        virtual ~ScriptPatternElement()
        {
            memLogger->debug("~ScriptPatternElement()");
        }

    private:
        ScriptValue *m_value;
        int m_repeatCount;
    };

    class ScriptPatternValue : public ScriptValue
    {
    public:
        ScriptPatternValue()
        {
            memLogger->debug("ScriptPatternValue()");
        }
        virtual ~ScriptPatternValue()
        {
            memLogger->debug("~ScriptPatternValue()");
        }
        virtual DRString toString()
        {
            return "ScriptPatternValue not implemented";
        }

    private:
        PtrList<ScriptPatternElement *> m_elements;
    };

    class ScriptCommand : public IScriptCommand
    {
    public:
        ScriptCommand(const char *type)
        {
            memLogger->debug("ScriptCommand():%s", type);
            m_logger = &ScriptCommandLogger;
            m_position = NULL;
            m_type = m_type;
        }
        virtual ~ScriptCommand()
        {
            memLogger->debug("~ScriptCommand()");
            delete m_position;
        }
        void step(ScriptState &state)
        {
            startStep(state);
            doStep(state);
            finishStep(state);
        }

        void setScriptPosition(IScriptPosition *position)
        {
            m_position = position;
        }

        const char * getType() { return m_type;}
    protected:
        virtual void startStep(ScriptState &state)
        {
            if (m_position)
            {
                state.addPosition(m_position);
            }
        }

        virtual void doStep(ScriptState &state)=0;

        virtual void finishStep(ScriptState &state)
        {
            if (m_position)
            {
                state.removePosition(m_position);
            }
        }

        DRString m_type;
        Logger *m_logger;
        IScriptPosition *m_position;
    };

    class PositionCommand : public ScriptCommand
    {
    public:
        PositionCommand() : ScriptCommand("PositionCommand")
        {
        }

        void doStep(ScriptState& state) override {
            // nothinig
        }

        void finishStep(ScriptState &state) override
        {
            // do nothing.  prevent the base class from removing
            // the m_position member from the state's position list
        }
    };

 
    class ScriptValueCommand : public ScriptCommand, IScriptValueProvider
    {
    public:
        ScriptValueCommand(const char *type = "ScriptValueCommand") : ScriptCommand(type)
        {
            memLogger->debug("ScriptValueCommand()");
        }

        virtual ~ScriptValueCommand()
        {
            memLogger->debug("~ScriptValueCommand()");
        }

        virtual void step(ScriptState &state)
        {
            state.addValueProvider(this);
        }

        void add(const char *name, ScriptValue *value)
        {
            m_logger->debug("Add ScriptValueCommand value %s 0x%04x", name,value);
            // DRString val = value->toString();
            // m_logger->debug("got toString()");
            // m_logger->debug("%s ",val.get());
            m_values.add(new NameValue(name, value));
            m_logger->debug("added NameValue");
        }

        virtual bool hasValue(const char *name)
        {
            return m_values.first([name](NameValue *nv)
                                  { return Util::equal(name, nv->getName()); }) != NULL;
        }
        virtual ScriptValue *getValue(const char *name)
        {
            NameValue **foundPtr = m_values.first([name](NameValue *nv)
                                                  { return Util::equal(name, nv->getName()); });
            if (foundPtr != NULL)
            {
                NameValue *nameValue = *foundPtr;
                return nameValue->getValue();
            }
            return NULL;
        }

        virtual int getIntValue(const char *name, ScriptState *state, double rangePositionPercent, int defaultValue)
        {
            ScriptValue *value = getValue(name);
            if (value == NULL)
            {
                return state->getIntValue(name, rangePositionPercent, defaultValue);
            }
        }

        virtual double getFloatValue(const char *name, ScriptState *state, double rangePositionPercent, double defaultValue)
        {
            ScriptValue *value = getValue(name);
            if (value == NULL)
            {
                return state->getFloatValue(name, rangePositionPercent, defaultValue);
            }
        }

    protected:
        void doStep(ScriptState& state) {
            state.addValueProvider(this);
        }
    private:
        PtrList<NameValue *> m_values;
    };

    class ScriptParameterCommand : public ScriptCommand, IScriptValueProvider
    {
        ScriptParameterCommand(const char *type = "ScriptParameterCommand") : ScriptCommand(type)
        {
            memLogger->debug("ScriptParameterCommand()");
        }

        virtual ~ScriptParameterCommand()
        {
            memLogger->debug("~ScriptParameterCommand()");
        }
    };

    class HSLCommand : public ScriptCommand
    {
    public:
        HSLCommand(IScriptValue *h = NULL, IScriptValue *s = NULL, IScriptValue *l = NULL) : ScriptCommand("HSLCommand")
        {
            memLogger->debug("HSLCommand()");
            m_hue = h;
            m_saturation = s;
            m_lightness = l;
        }

        virtual ~HSLCommand()
        {
            memLogger->debug("~HSLCommand() start");
            delete m_hue;
            delete m_saturation;
            delete m_lightness;
            memLogger->debug("~HSLCommand() start");
        }

        void setHue(IScriptValue *hue) { m_hue = hue; }
        IScriptValue *getHue(IScriptValue *hue) { return m_hue; }
        void setLightness(IScriptValue *lightness) { m_lightness = lightness; }
        IScriptValue *getLightness(IScriptValue *lightness) { return m_lightness; }
        void setSaturation(IScriptValue *saturation) { m_saturation = saturation; }
        IScriptValue *getSaturation(IScriptValue *saturation) { return m_saturation; }

        void doStep(ScriptState &state) override
        {
            IHSLStrip *strip = state.getStrip();
            int count = strip->getCount();
            if (count == 0)
            {
                m_logger->periodic(0, 1000, NULL, "strip has 0 LEDS");
                return;
            }
            Animator animate(0,count);
            for (int i = 0; i < count; i++)
            {
                double pct = animate.getValuePercent(i);
                int h = m_hue ? m_hue->getIntValue(state, pct,-1) : -1;
                strip->setHue(i, h, REPLACE);
                int l = m_lightness ? m_lightness->getIntValue(state, pct,-1) : -1;
                strip->setLightness(i, l, REPLACE);
                int s = m_saturation ? m_saturation->getIntValue(state, pct,-1) : -1;
                strip->setSaturation(i, s, REPLACE);
            }
        }


    private:
        IScriptValue *m_hue;
        IScriptValue *m_saturation;
        IScriptValue *m_lightness;
    };

    class RGBCommand : public ScriptCommand
    {
    public:
        RGBCommand(IScriptValue *r = 0, IScriptValue *g = 0, IScriptValue *b = 0) : ScriptCommand("RGBCommand")
        {
            memLogger->debug("RGBCommand()");
            m_red = r;
            m_green = g;
            m_blue = b;
        }

        virtual ~RGBCommand()
        {
            memLogger->debug("~RGBCommand() start");
            delete m_red;
            delete m_green;
            delete m_blue;
            memLogger->debug("~RGBCommand() done");
        }

        void setRed(IScriptValue *red) { m_red = red; }
        IScriptValue *getRed(IScriptValue *red) { return m_red; }
        void setGreen(IScriptValue *green) { m_green = green; }
        IScriptValue *getGreen(IScriptValue *green) { return m_green; }
        void setBlue(IScriptValue *blue) { m_blue = blue; }
        IScriptValue *getBlue(IScriptValue *blue) { return m_blue; }

        void doStep(ScriptState &state) override
        {
            IHSLStrip *strip = state.getStrip();
            int count = strip->getCount();
            if (count == 0)
            {
                m_logger->periodic(0, 1000, NULL, "strip has 0 LEDS");
                return;
            }
            Animator animate(0,count);
            for (int i = 0; i < count; i++)
            {
                double pct = animate.getValuePercent(i);
                m_logger->debug("RGB Led 0x%04, 0x%04, 0x%04",m_red,m_green,m_blue);
                int r = m_red ? m_red->getIntValue(state, pct,0) : 0;
                m_logger->debug("\tred=%d",r);
                int g = m_green? m_green->getIntValue(state, pct,0) : 0;
                m_logger->debug("\tgreen=%d",g);
                int b =  m_blue ? m_blue->getIntValue(state, pct,0) : 0;
                m_logger->debug("\tblue=%d",b);
                CRGB crgb(r,g,b);
                strip->setRGB(i, crgb, REPLACE);
            }
        }

        /*
                virtual void step(ScriptState &state)
                {
                    int pos = 0;
                    state.eachLed([&](IHSLStrip *strip, int idx)
                        {
                            m_logger->never("RGB Led 0x%04, 0x%04, 0x%04",m_red,m_green,m_blue);
                            int r = m_red ? m_red->getIntValue(state, pos,0) : 0;
                            m_logger->never("\tred=%d",r);
                            int g = m_green? m_green->getIntValue(state, pos,0) : 0;
                            m_logger->never("\tgreen=%d",g);
                            int b =  m_blue ? m_blue->getIntValue(state, pos,0) : 0;
                            m_logger->never("\tblue=%d",b);
                            CRGB crgb(r,g,b);
                            m_logger->never("\tgot crgb");
                            strip->setRGB(idx, crgb);
                            m_logger->never("\tstrip LED set");

                    });
                }
        */
    private:
        IScriptValue *m_red;
        IScriptValue *m_blue;
        IScriptValue *m_green;
    };

    class Script
    {
    public:
        Script()
        {
            memLogger->debug("Script()");
            m_logger = &ScriptLogger;
            m_frequencyMSecs = 50;
        }

        virtual ~Script()
        {
            memLogger->debug("~Script()");
        }

        void clear()
        {
            m_commands.clear();
        }

        void add(ScriptCommand *cmd)
        {
            m_commands.add(cmd);
        }

        void step(ScriptState &state)
        {
            if (m_frequencyMSecs > state.msecsSinceLastStep())
            {
                //m_logger->periodic(INFO_LEVEL,5000,NULL,"frequency not reached %d %d",m_frequencyMSecs,state.msecsSinceLastStep());
                return;
            }
            m_logger->never("reached step frequency");
            IHSLStrip * strip = state.getStrip();
            strip->clear();
            state.beginStep();

            m_commands.each([&](ScriptCommand *cmd) { 
                state.beginCommand(cmd);
                cmd->step(state); 
                state.endCommand(cmd); 
            });
            
            state.endStep();
            strip->show();
        }

        void setName(const char *name) { m_name = name; }
        const char *getName() { return m_name.text(); }
        void setFrequencyMSec(int msecs) { m_frequencyMSecs = msecs; }
        int getFrequencyMSec() { return m_frequencyMSecs; }

    private:
        Logger *m_logger;
        PtrList<ScriptCommand *> m_commands;
        DRString m_name;
        int m_frequencyMSecs;
    };

    void ScriptState::eachLed(auto &&lambda)
    {
        int count = m_strip->getCount();
        for (int i = 0; i < count; i++)
        {
            lambda(m_strip, i);
        }
    }

    void ScriptPosition::updateValues(ScriptState &state, IHSLStrip *strip)
    {
        ScriptLogger.test("updateValues strip");
        if (m_wrapValue) {
            m_wrap = m_wrapValue->getBoolValue(state,0,true);
        } else {
            m_wrap = true;
        }
        if (m_reverseValue) {
            m_reverse = m_reverseValue->getBoolValue(state,0,true);
        } else {
            m_reverse = false;
        }
        if (m_skipValue) {
            int skip  = m_skipValue->getIntValue(state,0,1);
            m_skip = skip;
            ScriptLogger.test("skip %d %d.  %s",skip,m_skip,m_skipValue->toString().get());
        } else {
            m_skip = 1;
        }

        m_base = strip;
        if (m_type == POS_ABSOLUTE){
            m_base = state.getBaseStrip();
        }
        m_previous = strip;
        m_start = 0;
        if (m_startValue != NULL)
        {
            m_start = m_startValue->getIntValue(state, 0, m_start);
        }
        m_count = strip->getCount();
        if (m_countValue != NULL)
        {
            m_count = m_countValue->getIntValue(state, 0, m_count);
            m_end = m_start+m_count-1;
        }
        else if (m_endValue != NULL)
        {
            m_end = m_start + m_count-1;
            m_end = m_endValue->getIntValue(state, 0, m_end);
            m_count = abs(m_end - m_start)+1;
        };
        if(m_reverse) {
            int tmp = m_start;
            m_start = m_end;
            m_end = tmp;
        }
        if (m_unit == POS_PERCENT) {
            double baseCount = m_base->getCount();
            m_start = round(m_start*100.0/baseCount);
            m_end = round(m_end*100.0/baseCount);
            m_count = round(m_count*100.0/baseCount);
        }
        m_animationOffset = 0;
        if (m_animation && m_count>0) {
            m_animationOffset = m_animation->getTimeValue(state, 0,m_count-1);
            ScriptLogger.test("animation offset: %d",m_animationOffset);
        }
        ScriptLogger.test("\tstart: %d. count %d. end %d. skip %d. wrap: %s.  reverse: %s.",m_start,m_count,m_end,m_skip,(m_wrap?"true":"false"),(m_reverse?"true":"false"));

    }
}
#endif