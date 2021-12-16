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
#include "../animation.h";
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
        virtual int getIntValue(ScriptState &state, int rangePositionPercent, int defaultValue) = 0;         // percent in [0,100]
        virtual double getFloatValue(ScriptState &state, int rangePositionPercent, double defaultValue) = 0; // percent in [0,100]

        // for debugging
        virtual DRString toString() = 0;
    };

    class IScriptValueProvider
    {
    public:
        virtual bool hasValue(const char *name) = 0;
        virtual IScriptValue *getValue(const char *name) = 0;
    };

    class NameValue
    {
    public:
        NameValue(const char *name, IScriptValue *value)
        {
            ScriptMemoryLogger.debug("create NameValue %s", name);
            m_name = name;
            m_value = value;
        }

        virtual ~NameValue()
        {
            ScriptMemoryLogger.debug("virtual ~NameValue() %s", m_name.text());
        }

        const char *getName() { return m_name.text(); }
        IScriptValue *getValue() { return m_value; }

    private:
        DRString m_name;
        IScriptValue *m_value;
    };

    class IScriptPosition : public IHSLStrip
    {
    public:
        virtual IHSLStrip *getPrevious() = 0;
        virtual void setPrevious(ScriptState &state, IHSLStrip *) = 0;
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
            memLogger->debug("virtual ~ScriptState");
        }

        void eachLed(auto &&lambda);

        void addValueProvider(IScriptValueProvider *provider)
        {
            m_valueProviders.insertAt(0, provider);
        }

        int getIntValue(const char *name, int rangePositionPercent, int defaultValue)
        {
            IScriptValue *value = NULL;
            IScriptValueProvider **providerPtr = m_valueProviders.first([&](IScriptValueProvider *p)
                                                                        {
                IScriptValue* v = p->getValue(name);
                if (v != NULL && m_lookupPath.firstIndexOf(v)<0) {
                    value = v;
                    return true;
                }        
                return false; });

            if (value == NULL)
            {
                m_logger->error("ScriptValueProvider does not have value named %s", name);
            }
            else
            {
                m_lookupPath.add(value);
                int i = value->getIntValue(*this, rangePositionPercent, defaultValue);
                m_lookupPath.removeFirst(value);
                // m_logger->debug("return value %d",i);
                return i;
            }
            return defaultValue;
        }

        double getFloatValue(const char *name, int rangePositionPercent, double defaultValue)
        {
            IScriptValueProvider **providerPtr = m_valueProviders.first([&](IScriptValueProvider *p)
                                                                        { return p->hasValue(name); });
            if (providerPtr != NULL)
            {
                IScriptValueProvider *provider = *providerPtr;
                IScriptValue *value = provider->getValue(name);
                if (value == NULL)
                {
                    m_logger->errorNoRepeat("ScriptValueProvider does not have value named %s", name);
                }
                else
                {
                    return value->getFloatValue(*this, rangePositionPercent, defaultValue);
                }
            }
            return defaultValue;
        }

        void beginScript(Script *script, IHSLStrip *strip)
        {
            m_script = script;
            m_strip = strip;
            m_startTime = millis();
            m_lastStepTime = 0;
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
            long now = millis();
            m_elapsedTime = now - m_lastStepTime;
            m_lastStepTime = now;
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

        long msecsSinceLastStep() { return m_elapsedTime; }
        long secondsSinceLastStep() { return m_elapsedTime; }

        void addPosition(IScriptPosition *position)
        {
            
            if (m_position != NULL)
            {
                position->setPrevious(*this, m_position);
            }
            else
            {
                position->setPrevious(*this, m_strip);
            }
            m_position = position;
        }

        void removePosition(IScriptPosition *position)
        {
            
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

        IHSLStrip *getStrip() { return m_strip; }

    private:
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
        long m_startTime;
        long m_lastStepTime;
        long m_elapsedTime;
        Script *m_script;
    };

    class ValueAnimator : public IValueAnimator
    {
    };

    class TimeValueAnimator : public ValueAnimator
    {
        // wrap - index wraps at end
        // repeat - when start reaches end, go back to start
        // delay - time to wait before repeat
        // unfold
    };

    class PositionValueAnimator : public ValueAnimator
    {
        // unfold
    };

    class ScriptPosition : public IScriptPosition
    {
    public:
        ScriptPosition()
        {
            m_startValue = NULL;
            m_endValue = NULL;
            m_countValue = NULL;
            m_skipValue = NULL;
            m_unit = POS_PERCENT;
            m_type = POS_RELATIVE;
            m_previous = NULL;
            m_start = 0;
            m_count = 0;
            m_wrap = true;
        }

        void setPrevious(ScriptState &state, IHSLStrip *strip);
        IHSLStrip *getPrevious()
        {
            return m_previous;
        }

        void setStartValue(IScriptValue *val) { m_startValue = val; }
        void setCountValue(IScriptValue *val) { m_countValue = val; }
        void setEndValue(IScriptValue *val) { m_endValue = val; }
        void setSkipValue(IScriptValue *val) { m_skipValue = val; }
        void setUnit(PositionUnit unit) { m_unit = unit; }
        void setHue(int index, int16_t hue, HSLOperation op = REPLACE)
        {
            m_previous->setHue(translate(index), hue, op);
        }
        void setSaturation(int index, int16_t saturation, HSLOperation op = REPLACE)
        {
            m_previous->setSaturation(translate(index), saturation, op);
        }
        void setLightness(int index, int16_t lightness, HSLOperation op = REPLACE)
        {
            m_previous->setLightness(translate(index), lightness, op);
        }
        void setRGB(int index, const CRGB &rgb, HSLOperation op = REPLACE)
        {
            m_previous->setRGB(translate(index), rgb, op);
        }
        size_t getCount() { return m_count; }
        size_t getStart() { return m_start; }
        int translate(int index)
        {

            if (m_unit == POS_PIXEL)
            {
                return index + m_start;
            }
            int count = m_count;
            if (index <= 0)
            {
                return 0;
            }
            if (index >= 100)
            {
                return count;
            }
            return (int)round((index * count) / 100.0) + m_start;
        }

        void setWrap(bool wrap) { m_wrap = wrap; }
        void clear() { m_previous->clear();}
        void show() { m_previous->show();}
    private:
        int m_start;
        int m_count;
        bool m_wrap;
        IScriptValue *m_startValue;
        IScriptValue *m_endValue;
        IScriptValue *m_countValue;
        IScriptValue *m_skipValue;
        PositionUnit m_unit;
        PositionType m_type;
        IHSLStrip *m_previous;
    };

    class ScriptVariableValue : public IScriptValue
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
            memLogger->debug("virtual ~ScriptVariableValue");
        }
        virtual int getIntValue(ScriptState &state, int percent, int defaultValue) override
        {
            int val = state.getIntValue(m_name.text(), percent, m_defaultValue);
            m_logger->never("ScriptVariableValue %s got %d", m_name.text(), val);
            return val;
        }

        virtual double getFloatValue(ScriptState &state, int percent, double defaultValue) override
        {
            double val = state.getFloatValue(m_name.text(), percent, m_defaultValue);
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

    // ScriptVariableGenerator: ??? rand, trig, ...

    class ScriptFunctionValue : public IScriptValue
    {
        // todo: named functions, call?
    public:
        ScriptFunctionValue(const char *value) : m_value(value)
        {
            memLogger->debug("ScriptVariableValue()");
        }

        virtual ~ScriptFunctionValue()
        {
            memLogger->debug("virtual ~ScriptFunctionValue()");
        }
        virtual int getIntValue(ScriptState &state, int percent, int defaultValue) override
        {
            return defaultValue;
        }

        virtual double getFloatValue(ScriptState &state, int percent, double defaultValue) override
        {
            return defaultValue;
        }
        virtual DRString toString() { return DRString("Function: ").append(m_value); }

    protected:
        DRString m_value;
    };

    class ScriptNumberValue : public IScriptValue
    {
    public:
        ScriptNumberValue(double value) : m_value(value)
        {
            memLogger->debug("ScriptNumberValue()");
        }

        virtual ~ScriptNumberValue()
        {
            memLogger->debug("virtual ~ScriptNumberValue()");
        }

        virtual int getIntValue(ScriptState &state, int percent, int defaultValue) override
        {
            return roundl(m_value);
        }

        virtual double getFloatValue(ScriptState &state, int percent, double defaultValue) override
        {
            return m_value;
        }

        virtual DRString toString() { return DRString::fromFloat(m_value); }

    protected:
        double m_value;
    };

    class ScriptStringValue : public IScriptValue
    {
    public:
        ScriptStringValue(const char *value) : m_value(value)
        {
            memLogger->debug("ScriptStringValue()");
        }

        virtual ~ScriptStringValue()
        {
            memLogger->debug("virtual ~ScriptStringValue()");
        }

        virtual int getIntValue(ScriptState &state, int percent, int defaultValue) override
        {
            const char *n = m_value.text();
            if (n != NULL)
            {
                return atoi(n);
            }
            return defaultValue;
        }

        virtual double getFloatValue(ScriptState &state, int percent, double defaultValue) override
        {
            const char *n = m_value.text();
            if (n != NULL)
            {
                return atof(n);
            }
            return defaultValue;
        }

        virtual DRString toString() { return m_value; }

    protected:
        DRString m_value;
    };

    class ScriptRangeValue : public IScriptValue
    {
    public:
        ScriptRangeValue(IScriptValue *start, IScriptValue *end)
        {
            memLogger->debug("ScriptRangeValue()");
            m_start = start;
            m_end = end;
        }

        virtual ~ScriptRangeValue()
        {
            memLogger->debug("virtual ~ScriptRangeValue() start");
            delete m_start;
            delete m_end;
            memLogger->debug("virtual ~ScriptRangeValue() end");
        }

        virtual int getIntValue(ScriptState &state, int percent, int defaultValue)
        {
            if (m_start == NULL)
            {
                return m_end ? m_end->getIntValue(state, 100, defaultValue) : defaultValue;
            }
            else if (m_end == NULL)
            {
                return m_start ? m_start->getIntValue(state, 0, defaultValue) : defaultValue;
            }
            int start = m_start->getIntValue(state, percent, 0);
            int end = m_end->getIntValue(state, percent, 100);
            int diff = end - start;
            int result = start + diff * percent / 100;
            return result;
        }

        virtual double getFloatValue(ScriptState &state, int percent, double defaultValue)
        {
            if (m_start == NULL)
            {
                return m_end ? m_end->getIntValue(state, 100, defaultValue) : defaultValue;
            }
            else if (m_end == NULL)
            {
                return m_start ? m_start->getIntValue(state, 0, defaultValue) : defaultValue;
            }
            double start = m_start->getFloatValue(state, percent, 0);
            double end = m_end->getFloatValue(state, percent, 100);
            double diff = end - start;
            double result = start + diff * percent / 100;
            return result;
        }

        virtual DRString toString()
        {
            DRString result("range:");
            result.append(m_start ? m_start->toString() : "NULL")
                .append(m_end ? m_end->toString() : "NULL");
        }

    protected:
        IScriptValue *m_start;
        IScriptValue *m_end;
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
            memLogger->debug("virtual ~ScriptPatternElement()");
        }

    private:
        IScriptValue *m_value;
        int m_repeatCount;
    };

    class ScriptPatternValue : public IScriptValue
    {
    public:
        ScriptPatternValue()
        {
            memLogger->debug("ScriptPatternValue()");
        }
        virtual ~ScriptPatternValue()
        {
            memLogger->debug("virtual ~ScriptPatternValue()");
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
        }
        virtual ~ScriptCommand()
        {
            memLogger->debug("virtual ~ScriptCommand()");
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

    protected:
        virtual void startStep(ScriptState &state)
        {
            if (m_position)
            {
                state.addPosition(m_position);
            }
        }

        virtual void doStep(ScriptState &state)
        {
        }

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

        virtual void finishStep(ScriptState &state) override
        {
            // do nothing.  prevent the base class from removing
            // the m_position member from the state's position list
        }
    };

    class TimeAnimatorCommand : public ScriptCommand
    {
    private:
        ValueAnimator *m_animator;
    };

    class PositionAnimatorCommand : public ScriptCommand
    {
    private:
        ValueAnimator *m_animator;
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
            memLogger->debug("virtual ~ScriptValueCommand()");
        }

        virtual void step(ScriptState &state)
        {
            state.addValueProvider(this);
        }

        void add(const char *name, IScriptValue *value)
        {
            m_logger->debug("Add ScriptValueCommand value %s=%s", name, value->toString().get());
            m_values.add(new NameValue(name, value));
        }

        virtual bool hasValue(const char *name)
        {
            return m_values.first([name](NameValue *nv)
                                  { return Util::equal(name, nv->getName()); }) != NULL;
        }
        virtual IScriptValue *getValue(const char *name)
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

        virtual int getIntValue(const char *name, ScriptState *state, int rangePositionPercent, int defaultValue)
        {
            IScriptValue *value = getValue(name);
            if (value == NULL)
            {
                return state->getIntValue(name, rangePositionPercent, defaultValue);
            }
        }

        virtual double getFloatValue(const char *name, ScriptState *state, int rangePositionPercent, double defaultValue)
        {
            IScriptValue *value = getValue(name);
            if (value == NULL)
            {
                return state->getFloatValue(name, rangePositionPercent, defaultValue);
            }
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
            memLogger->debug("virtual ~ScriptParameterCommand()");
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
            memLogger->debug("virtual ~HSLCommand() start");
            delete m_hue;
            delete m_saturation;
            delete m_lightness;
            memLogger->debug("virtual ~HSLCommand() start");
        }

        void setHue(IScriptValue *hue) { m_hue = hue; }
        IScriptValue *getHue(IScriptValue *hue) { return m_hue; }
        void setLightness(IScriptValue *lightness) { m_lightness = lightness; }
        IScriptValue *getLightness(IScriptValue *lightness) { return m_lightness; }
        void setSaturation(IScriptValue *saturation) { m_saturation = saturation; }
        IScriptValue *getSaturation(IScriptValue *saturation) { return m_saturation; }

        virtual void step(ScriptState &state)
        {
            int pos = 0;
            state.eachLed([&](IHSLStrip *strip, int idx)
                          {
                    if (m_hue >= 0) {
                        strip->setHue(idx,m_hue->getIntValue(state,pos,0));
                    }
                    if (m_saturation>= 0) {
                        strip->setSaturation(idx,m_saturation->getIntValue(state,pos,0));
                    }
                    if (m_lightness>= 0) {
                        strip->setLightness(idx,m_lightness->getIntValue(state,pos,0));
                    } });
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
            memLogger->debug("virtual ~RGBCommand() start");
            delete m_red;
            delete m_green;
            delete m_blue;
            memLogger->debug("virtual ~RGBCommand() done");
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
            for (int i = 0; i < count; i++)
            {
                int pos = i*100/count;
                m_logger->never("RGB Led 0x%04, 0x%04, 0x%04",m_red,m_green,m_blue);
                int r = m_red ? m_red->getIntValue(state, pos,0) : 0;
                m_logger->never("\tred=%d",r);
                int g = m_green? m_green->getIntValue(state, pos,0) : 0;
                m_logger->never("\tgreen=%d",g);
                int b =  m_blue ? m_blue->getIntValue(state, pos,0) : 0;
                m_logger->never("\tblue=%d",b);
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
            memLogger->debug("virtual ~Script()");
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
                return;
            }
            m_logger->debug("reached step frequency");
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

    void ScriptPosition::setPrevious(ScriptState &state, IHSLStrip *strip)
    {
        m_previous = strip;
        m_start = strip->getStart();
        if (m_startValue != NULL)
        {
            m_start = m_startValue->getIntValue(state, 0, m_start);
        }
        m_count = strip->getCount();
        if (m_countValue != NULL)
        {
            m_count = m_countValue->getIntValue(state, 0, m_count);
        }
        else if (m_endValue != NULL)
        {
            int end = m_start + m_count;
            end = m_endValue->getIntValue(state, 0, end);
            m_count = end - m_start;
        };
    }
}
#endif