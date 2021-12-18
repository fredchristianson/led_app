#ifndef DRSCRIPT_VALUE_H
#define DRSCRIPT_VALUE_H

#include "../logger.h"
#include "../led_strip.h"
#include "../list.h"
#include "../ensure.h"
#include "./script_interface.h"

namespace DevRelief
{

    class ScriptValue : public IScriptValue {
        public:
            ScriptValue() {
                m_logger = &ScriptCommandLogger;
            }
            virtual ~ScriptValue() {}

            void destroy() override { delete this;}

            bool isRecursing() override { return false;}
        protected:
            Logger* m_logger;
    };
  
    class NameValue
    {
    public:
        NameValue(const char *name, IScriptValue *value)
        {
            ScriptMemoryLogger.debug("NameValue %s", name);
            m_name = name;
            m_value = value;
        }

        virtual ~NameValue()
        {
            ScriptMemoryLogger.debug("~NameValue() %s 0x%04X", m_name.text(),m_value);
            m_value->destroy();
        }

        const char *getName() { return m_name.text(); }
        IScriptValue *getValue() { return m_value; }

    private:
        DRString m_name;
        IScriptValue *m_value;
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
        int getIntValue(IScriptCommand* cmd,  int defaultValue) override
        {
            return defaultValue;
        }

        double getFloatValue(IScriptCommand* cmd,  double defaultValue) override
        {
            return defaultValue;
        }

       bool getBoolValue(IScriptCommand* cmd,  bool defaultValue) override
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

        virtual int getIntValue(IScriptCommand* cmd,  int defaultValue) override
        {
            int v = m_value;
            m_logger->test("getIntValue %f %d",m_value,v);
            return v;
        }

        virtual double getFloatValue(IScriptCommand* cmd,  double defaultValue) override
        {
            return m_value;
        }

        virtual bool getBoolValue(IScriptCommand* cmd,  bool defaultValue) override
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

        int getIntValue(IScriptCommand* cmd,  int defaultValue) override
        {
            return m_value ? 1 : 0;
        }

        double getFloatValue(IScriptCommand* cmd,  double defaultValue) override
        {
            return m_value ? 1 : 0;
        }

        bool getBoolValue(IScriptCommand* cmd,  bool defaultValue) override {
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

        int getIntValue(IScriptCommand* cmd,  int defaultValue) override
        {
            const char *n = m_value.text();
            if (n != NULL)
            {
                return atoi(n);
            }
            return defaultValue;
        }

        double getFloatValue(IScriptCommand* cmd,  double defaultValue) override
        {
            const char *n = m_value.text();
            if (n != NULL)
            {
                return atof(n);
            }
            return defaultValue;
        }

        bool getBoolValue(IScriptCommand* cmd,  bool defaultValue) override
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
        ScriptRangeValue(IScriptValue *start, IScriptValue *end)
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

        virtual int getIntValue(IScriptCommand* cmd,  int defaultValue)
        {
            return (int)getFloatValue(cmd,(double)defaultValue);
        }

        virtual double getFloatValue(IScriptCommand* cmd,  double defaultValue)
        {
            /*
            int percent = 0;  // todo: fix for new command/state impl
            if (m_start == NULL)
            {
                return m_end ? m_end->getIntValue(cmd, defaultValue) : defaultValue;
            }
            else if (m_end == NULL)
            {
                return m_start ? m_start->getIntValue(cmd, defaultValue) : defaultValue;
            }
            double start = m_start->getIntValue(cmd, 0);
            double end = m_end->getIntValue(cmd,  1);
            m_logger->test("check canAnimateOverTime");
            if (m_animate == NULL) {
                int diff = end - start;
                int result = start + diff * percent;
                return result;
            } else {
                if (m_animate->canAnimateOverTime()){
                    m_logger->test("animator has speed (animate over time");
                    return m_animate->getTimeValue(state,start,end);
                } else {
                    m_logger->test("\tuse position 0x%04X",m_animate);
                    return m_animate->getPositionValue(state,start,end,percent);

                }
            }
            */
           return 0;
        }
        virtual bool getBoolValue(IScriptCommand* cmd,  bool defaultValue)
        {
            int start = m_start->getIntValue(cmd, 0);
            int end = m_end->getIntValue(cmd, start);
            // bool probably doesn't make sense for a range.  return true if there is a range rather than single value
           return start != end;
        }

        virtual DRString toString()
        {
            DRString result("range:");
            result.append(m_start ? m_start->toString() : "NULL")
                .append(m_end ? m_end->toString() : "NULL");
        }

        void setAnimator(IValueAnimator* animator) {
            m_animate = animator;
        }
    protected:
        IScriptValue *m_start;
        IScriptValue *m_end;
        IValueAnimator* m_animate;
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

    class ScriptVariableValue : public IScriptValue
    {
    public:
        ScriptVariableValue(const char *value) : m_name(value)
        {
            memLogger->debug("ScriptVariableValue()");
            m_logger = &ScriptLogger;
            m_logger->debug("Created ScriptVariableValue %s.", value);
            m_hasDefaultValue = false;
            m_recurse = false;
        }

        virtual ~ScriptVariableValue()
        {
            memLogger->debug("~ScriptVariableValue");
        }

        void setDefaultValue(double val) {
            m_defaultValue = val;
            m_hasDefaultValue = true;
        }

        void destroy() override { delete this;}
        virtual int getIntValue(IScriptCommand*cmd,  int defaultValue) override
        {
            m_recurse = true;
            IScriptValue * val = cmd->getValue(m_name);
            int dv = m_hasDefaultValue ? m_defaultValue : defaultValue;
            if (val != NULL) {
                dv = val->getIntValue(cmd,dv);
            }
            m_recurse = false;
            return dv;
        }

        virtual double getFloatValue(IScriptCommand*cmd,  double defaultValue) override
        {
            m_recurse = true;
            IScriptValue * val = cmd->getValue(m_name);
            int dv = m_hasDefaultValue ? m_defaultValue : defaultValue;
            if (val != NULL) {
                dv = val->getFloatValue(cmd,dv);
            }
            m_recurse = false;
            return dv;
        }

        virtual bool getBoolValue(IScriptCommand*cmd,  bool defaultValue) override
        {
            m_recurse = true;
            IScriptValue * val = cmd->getValue(m_name);
            bool dv = m_hasDefaultValue ? (m_defaultValue != 0) : defaultValue;
            if (val != NULL) {
                dv = val->getBoolValue(cmd,dv);
            }
            m_recurse = false;
            return dv;
        }

        virtual DRString toString() { return DRString("Variable: ").append(m_name); }

        bool isRecursing() { return m_recurse;}
    protected:
        DRString m_name;
        bool m_hasDefaultValue;
        double m_defaultValue;
        Logger *m_logger;
        bool m_recurse;
    };

    class ScriptValueList : public IScriptValueProvider {
        public:
            ScriptValueList() {
                m_logger = &ScriptLogger;
                m_logger->info("create ScriptValueList()");
            }

            virtual ~ScriptValueList() {
                m_logger->info("delete ~ScriptValueList()");
            }

            bool hasValue(const char *name) override  {
                return getValue(name) != NULL;
            }
            
            IScriptValue *getValue(const char *name)  override {
                NameValue** first = m_values.first([&](NameValue*&nv) {
                    if (!Ensure::notNull(nv,NULL,"NameValue cannot be NULL")){
                        return false;
                    }
                    return strcmp(nv->getName(),name)==0;
                });
                return first ? (*first)->getValue() : NULL;
            }

            void addValue(const char * name,IScriptValue * value) {
                if (Util::isEmpty(name) || value == NULL) {
                    return;
                }
                m_logger->debug("add NameValue %s  0x%04X",name,value);
                NameValue* nv = new NameValue(name,value);
                m_values.add(nv);
            }
 
            void destroy() { delete this;}
        private:
            PtrList<NameValue*> m_values;
            Logger* m_logger;
   };
}
#endif