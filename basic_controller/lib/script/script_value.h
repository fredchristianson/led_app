#ifndef DRSCRIPT_VALUE_H
#define DRSCRIPT_VALUE_H

#include "../logger.h"
#include "../led_strip.h"
#include "../list.h"
#include "../ensure.h"
#include "./script_interface.h"
#include "./animation.h"

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

            bool isString(IScriptCommand* cmd)  override{
              return false;  
            } 
            bool isNumber(IScriptCommand* cmd)  override{
              return false;  
            } 
            bool isBool(IScriptCommand* cmd)  override{
              return false;  
            } 

            bool isNull(IScriptCommand* cmd)  override{
              return false;  
            } 

            bool equals(IScriptCommand*cmd,const char * match) override { return false;}

            IScriptValue* eval(IScriptCommand * cmd, double defaultValue=0) override;
            
        protected:
            Logger* m_logger;
    };

    class ScriptSystemValue : public ScriptValue {
        public:
            ScriptSystemValue(const char* name) {
                DRStringBuffer buf;
                auto parts = buf.split(name,":");
                if (buf.count() == 2) {
                    m_logger->always("got name and scope");
                    m_scope = buf.getAt(0);
                    m_name = buf.getAt(1);
                } else {
                    m_name = buf.getAt(0);
                }
            }

            int getIntValue(IScriptCommand* cmd,  int defaultValue) override
            {
                return (int)getFloatValue(cmd,(double)defaultValue);
            }

            double getFloatValue(IScriptCommand* cmd,  double defaultValue) override
            {
                return get(cmd,defaultValue);
            }

        bool getBoolValue(IScriptCommand* cmd,  bool defaultValue) override
            {
                double d = getFloatValue(cmd,0);
                return d != 0;
            }
        
        int getMsecValue(IScriptCommand* cmd,  int defaultValue) override { return defaultValue;}
        bool isNumber(IScriptCommand* cmd) override { return true;}



        DRString toString() { return DRString("System Value: ").append(m_name); }
        private:
            double get(IScriptCommand* cmd, double defaultValue){
                double val = defaultValue;
                if (Util::equal(m_name,"start")) {
                    val = cmd->getState()->getStrip()->getStart();
                } else if (Util::equal(m_name,"count")) {
                    val = cmd->getState()->getStrip()->getCount();
                } else if (Util::equal(m_name,"step")) {
                    val = cmd->getState()->getStepNumber();
                } else if (Util::equal(m_name,"red")) {
                    val = HUE::RED;
                }  else if (Util::equal(m_name,"orange")) {
                    val = HUE::ORANGE;
                }  else if (Util::equal(m_name,"yellow")) {
                    val = HUE::YELLOW;
                }  else if (Util::equal(m_name,"green")) {
                    val = HUE::GREEN;
                }  else if (Util::equal(m_name,"cyan")) {
                    val = HUE::CYAN;
                }  else if (Util::equal(m_name,"blue")) {
                    val = HUE::BLUE;
                }  else if (Util::equal(m_name,"magenta")) {
                    val = HUE::MAGENTA;
                }  else if (Util::equal(m_name,"purple")) {
                    val = HUE::PURPLE;
                } 


                m_logger->never("SystemValue %s:%s %f",m_scope.get(),m_name.get(),val);
                return val;
            }
            DRString m_scope;
            DRString m_name;


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

        virtual void destroy() { delete this;}

        const char *getName() { return m_name.text(); }
        IScriptValue *getValue() { return m_value; }

    private:
        DRString m_name;
        IScriptValue *m_value;
    };
    // ScriptVariableGenerator: ??? rand, trig, ...
    class FunctionArgs {
        public:
            FunctionArgs() {}
            virtual ~FunctionArgs() {}

            void add(IScriptValue* val) { args.add(val);}
            IScriptValue* get(int index) { return args.get(index);}
            size_t length() { return args.size();}
        PtrList<IScriptValue*> args;
    };

    int randTotal = millis();

    class ScriptFunction : public ScriptValue
    {
    public:
        ScriptFunction(const char *name, FunctionArgs* args) : m_name(name), m_args(args)
        {
            memLogger->debug("ScriptVariableValue()");
            randomSeed(analogRead(0)+millis());
            m_funcState = -1;
        }

        virtual ~ScriptFunction()
        {
            memLogger->debug("~ScriptFunction()");
            delete m_args;
        }
        int getIntValue(IScriptCommand* cmd,  int defaultValue) override
        {
            return (int)getFloatValue(cmd,(double)defaultValue);
        }

        double getFloatValue(IScriptCommand* cmd,  double defaultValue) override
        {
            return invoke(cmd,defaultValue);
        }

       bool getBoolValue(IScriptCommand* cmd,  bool defaultValue) override
        {
            double d = getFloatValue(cmd,0);
            return d != 0;
        }

        DRString toString() { return DRString("Function: ").append(m_name); }
        int getMsecValue(IScriptCommand* cmd,  int defaultValue) override { 
            if (Util::equal("millis",m_name)) {
                return millis();
            }
            return defaultValue;
        }
        bool isNumber(IScriptCommand* cmd) override { return true;}
        
    protected:
        double invoke(IScriptCommand * cmd,double defaultValue) {
            double result = 0;
            const char * name = m_name.get();
            if (Util::equal("rand",name)){
                result = invokeRand(cmd,defaultValue);
            } else if (Util::equal("add",name) || Util::equal("+",name)) {
                result = invokeAdd(cmd,defaultValue);
            } else if (Util::equal("subtract",name) ||Util::equal("sub",name) || Util::equal("-",name)) {
                result = invokeSubtract(cmd,defaultValue);
            } else if (Util::equal("multiply",name) || Util::equal("mult",name) || Util::equal("*",name)) {
                result = invokeMultiply(cmd,defaultValue);
            } else if (Util::equal("divide",name) || Util::equal("div",name) || Util::equal("/",name)) {
                result = invokeDivide(cmd,defaultValue);
            } else if (Util::equal("mod",name) || Util::equal("%",name)) {
                result = invokeMod(cmd,defaultValue);
            } else if (Util::equal("min",name)) {
                result = invokeMin(cmd,defaultValue);
            } else if (Util::equal("max",name)) {
                result = invokeMax(cmd,defaultValue);
            } else if (Util::equal("randOf",name)) {
                result = invokeRandomOf(cmd,defaultValue);
            } else if (Util::equal("seq",name)||Util::equal("sequence",name)) {
                result = invokeSequence(cmd,defaultValue);
            } else {
                m_logger->error("unknown function: %s",name);
            }
            m_logger->never("function: %s=%f",m_name.get(),result);
            return result;
        }

        double invokeRand(IScriptCommand*cmd ,double defaultValue) {
            int low = getArgValue(cmd,0,0);
            int high = getArgValue(cmd, 1,low);
            if (high == low) {
                low = 0;
            }
            if (high < low) {
                int t = low;
                low = high;
                high = t;
            }
            int val = random(low,high+1);
            return val;
            
        }

        double invokeAdd(IScriptCommand*cmd,double defaultValue) {
            double first = getArgValue(cmd,0,defaultValue);
            double second = getArgValue(cmd,1,defaultValue);
            return first + second;
        }

        double invokeSubtract(IScriptCommand*cmd,double defaultValue) {
            double first = getArgValue(cmd,0,defaultValue);
            double second = getArgValue(cmd,1,defaultValue);
            return first - second;
        }

        double invokeMultiply(IScriptCommand*cmd,double defaultValue) {
            double first = getArgValue(cmd,0,defaultValue);
            double second = getArgValue(cmd,1,defaultValue);
            return first * second;
        }

        double invokeDivide(IScriptCommand*cmd,double defaultValue) {
            double first = getArgValue(cmd,0,defaultValue);
            double second = getArgValue(cmd,1,defaultValue);
            return (second == 0) ? 0 : first / second;
        }

        
        double invokeMod(IScriptCommand*cmd,double defaultValue) {
            double first = getArgValue(cmd,0,defaultValue);
            double second = getArgValue(cmd,1,defaultValue);
            return (double)((int)first % (int)second);
        }
        
        double invokeMin(IScriptCommand*cmd,double defaultValue) {
            double first = getArgValue(cmd,0,defaultValue);
            double second = getArgValue(cmd,1,defaultValue);
            return first < second ? first : second;
        }
        
        double invokeMax(IScriptCommand*cmd,double defaultValue) {
            double first = getArgValue(cmd,0,defaultValue);
            double second = getArgValue(cmd,1,defaultValue);
            return first > second ? first : second;
        }

        double invokeRandomOf(IScriptCommand*cmd,double defaultValue) {
            int count = m_args->length();
            int idx = random(count);
            return getArgValue(cmd,idx,defaultValue);
        }

        double invokeSequence(IScriptCommand*cmd,double defaultValue) {
            
            int start = getArgValue(cmd,0,0);
            int end = getArgValue(cmd,1,100);
            int step = getArgValue(cmd,2,1);

            if (m_funcState < start){ 
                m_funcState = start;
            } else {
                m_funcState += step;
            }
            if (m_funcState > end){
                m_funcState = start;
            }

            m_logger->never("Sequence %d %d %d ==> %f",start,end,step,m_funcState);
            return m_funcState;
        }

        double getArgValue(IScriptCommand*cmd, int idx, double defaultValue){
            if (m_args == NULL) {
                return defaultValue;
            }
            IScriptValue* val = m_args->get(idx);
            if (val == NULL) { return defaultValue;}
            return val->getFloatValue(cmd,defaultValue);
        }

        DRString m_name;
        FunctionArgs * m_args;
        double m_funcState; // different functions can use in their way
    };

    class ScriptNumberValue : public ScriptValue
    {
    public:
        ScriptNumberValue(double value) : m_value(value)
        {
            memLogger->debug("ScriptNumberValue()");
        }

        ScriptNumberValue(IScriptCommand*cmd, IScriptValue* base,double defaultValue) {
            double val = base->getFloatValue(cmd,defaultValue);
            m_value = val;
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

        int getMsecValue(IScriptCommand* cmd,  int defaultValue) override { 
            return m_value;
        }
        bool isNumber(IScriptCommand* cmd) override { return true;}

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

        int getMsecValue(IScriptCommand* cmd,  int defaultValue) override { 
            return defaultValue;
        }
        bool isBool(IScriptCommand* cmd) override { return true;}

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

    
    class ScriptNullValue : public ScriptValue
    {
    public:
        ScriptNullValue() 
        {
            memLogger->debug("ScriptNullValue()");
            m_logger->debug("ScriptNullValue()");
        }

        virtual ~ScriptNullValue()
        {
            m_logger->debug("~ScriptNullValue()");
            memLogger->debug("~ScriptNullValue()");
        }

        int getIntValue(IScriptCommand* cmd,  int defaultValue) override
        {
            return defaultValue;
        }

        double getFloatValue(IScriptCommand* cmd,  double defaultValue) override
        {
            return defaultValue;
        }

        bool getBoolValue(IScriptCommand* cmd,  bool defaultValue) override {
            return defaultValue;
        }

        int getMsecValue(IScriptCommand* cmd,  int defaultValue) override { 
            return defaultValue;
        }

        bool isBool(IScriptCommand* cmd) override { return false;}
        bool isNull(IScriptCommand* cmd)  override{
            return true;  
        } 

        DRString toString() override { 
            m_logger->debug("ScriptNulllValue.toString()");
            DRString drv("ScriptNullValue");
            m_logger->debug("\tcreated DRString");
            return drv;
        }

    protected:

    };


    class ScriptStringValue : public ScriptValue
    {
    public:
        ScriptStringValue(const char *value) : m_value(value)
        {
            memLogger->debug("ScriptStringValue()");
            m_logger->debug("ScriptStringValue 0x%04X %s",this,value);
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

        bool equals(IScriptCommand*cmd, const char * match) override { 
            m_logger->never("ScriptStringValue.equals %s==%s",m_value.get(),match);
            return Util::equal(m_value.text(),match);
        }

        int getMsecValue(IScriptCommand* cmd,  int defaultValue) override { 
            return Util::toMsecs(m_value);
        }
        bool isString(IScriptCommand* cmd) override { return true;}

        DRString toString() override { return m_value; }

    protected:
        DRString m_value;
    };

    class ScriptRangeValue : public ScriptValue
    {
    public:
        ScriptRangeValue(IScriptValue *start, IScriptValue *end,IValueAnimator * animate=NULL )
        {
            memLogger->debug("ScriptRangeValue()");
            m_start = start;
            m_end = end;
            m_animate = animate;

        }

        virtual ~ScriptRangeValue()
        {
            memLogger->debug("~ScriptRangeValue() start");
            if (m_start) {m_start->destroy();}
            if (m_end) {m_end->destroy();}
            if (m_animate) {m_animate->destroy();}
            memLogger->debug("~ScriptRangeValue() end");
        }

        int getMsecValue(IScriptCommand* cmd,  int defaultValue) override { 
            return getIntValue(cmd,defaultValue);
        }
        bool isNumber(IScriptCommand* cmd) override { return true;}

        virtual int getIntValue(IScriptCommand* cmd,  int defaultValue)
        {
            return (int)getFloatValue(cmd,(double)defaultValue);
        }

        virtual double getFloatValue(IScriptCommand* cmd,  double defaultValue)
        {
            if (m_start == NULL)
            {
                m_logger->debug("\tno start.  return end %f");
                return m_end ? m_end->getIntValue(cmd, defaultValue) : defaultValue;
            }
            else if (m_end == NULL)
            {
                m_logger->debug("\tno end.  return start %f");
                return m_start ? m_start->getIntValue(cmd, defaultValue) : defaultValue;
            }
            double start = m_start->getFloatValue(cmd, 0);
            double end = m_end->getFloatValue(cmd,  1);
            double value = 0;
            IScriptState* state = cmd->getState();
            if (m_animate) {
                AnimationRange range(start,end);
                value = m_animate->get(cmd,range);
            } else {
                AnimationRange range(start,end,false);
                Animator animator(*(cmd->getAnimationPositionDomain()));
                CubicBezierEase ease;
                animator.setEase(&ease);

                value = animator.get(range,cmd);
                
            }
            m_logger->never("Range %f-%f got %f",start,end,value);
            return value;
  
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
            m_logger->never("format range DRString");
            DRString result("range:");
            result.append(m_start ? m_start->toString() : "NULL")
                .append("--")
                .append(m_end ? m_end->toString() : "NULL");
            return result;
        }

        void setAnimator(IValueAnimator* animator) {
            m_animate = animator;
        }

        IScriptValue* eval(IScriptCommand * cmd, double defaultValue=0) override{
            auto start = m_start ? m_start->eval(cmd,defaultValue) : NULL;
            auto end = m_end ? m_end->eval(cmd,defaultValue) : NULL;
            auto animator = m_animate ? m_animate->clone(cmd) : NULL;
            return new ScriptRangeValue(start,end,animator);
        }


    protected:
        IScriptValue *m_start;
        IScriptValue *m_end;

        IValueAnimator* m_animate;
    };



    class ScriptPatternElement
    {
    public:
        ScriptPatternElement(int repeatCount, IScriptValue* value)
        {
            memLogger->debug("ScriptPatternElement()");
            m_value = value;
            m_repeatCount = repeatCount;
            ScriptLogger.always("PatternElement %d %s",repeatCount,value->toString().text());
        }
        virtual ~ScriptPatternElement()
        {
            if (m_value) {m_value->destroy();}
            memLogger->debug("~ScriptPatternElement()");
        }

        int getRepeatCount() { return m_repeatCount;}

        IScriptValue* getValue() { return m_value;}
        virtual void destroy() { delete this;}

    private:
        IScriptValue *m_value;
        int m_repeatCount;
    };

    typedef enum PatternExtend {
        REPEAT_PATTERN=0,
        STRETCH_PATTERN=1,
        NO_EXTEND=2
    };

   class PatternValue : public ScriptValue
    {
    public:
        PatternValue(IValueAnimator* animate=NULL)
        {
            memLogger->debug("PatternValue()");
            m_animate = animate;
            m_extend = REPEAT_PATTERN;
            m_count = 0;

        }

        virtual ~PatternValue()
        {
            memLogger->debug("~PatternValue() start");
            if (m_animate) {m_animate->destroy();}
            memLogger->debug("~PatternValue() end");
        }

        void setExtend(PatternExtend val) {
            m_extend = val;
        }

        void addElement(ScriptPatternElement* element) {
            if (element == NULL) {
                m_logger->error("ScriptPatternElement cannot be NULL");
                return;
            }
            m_elements.add(element);
            m_count += element->getRepeatCount();
        }

        int getMsecValue(IScriptCommand* cmd,  int defaultValue) override { 
            return getIntValue(cmd,defaultValue);
        }
        bool isNumber(IScriptCommand* cmd) override { return false;}

        virtual int getIntValue(IScriptCommand* cmd,  int defaultValue)
        {
            return (int)getFloatValue(cmd,(double)defaultValue);
        }

        virtual double getFloatValue(IScriptCommand* cmd,  double defaultValue)
        {
            if (m_count == 0) { return defaultValue; }

            double pos = 0;
            AnimationDomain* domain = cmd->getAnimationPositionDomain();
            if (m_animate) {
                AnimationRange range(0,m_count);
                pos  = m_animate->get(cmd,range);
                domain = m_animate->getDomain(cmd,range);
 
            } else {
                pos = domain->getValue();
            }
            double val = getValueAt(cmd, domain, pos,defaultValue);
            return val;
        }

        double getValueAt(IScriptCommand* cmd,AnimationDomain* domain, int pos, double defaultValue) {
            if (pos < 0 || pos >= m_count) {
                if (m_extend == NO_EXTEND) {
                    return defaultValue;
                } else if (m_extend == REPEAT_PATTERN) {
                    pos = abs(pos)%m_count;
                }
            }
            
            int index = pos % m_count;
            if (m_extend == STRETCH_PATTERN) {
                double pct = (index*1.0/m_count);   
                double domainVal = domain->getPosition();
                index = m_count * domainVal;

            }
            int elementIndex = 0;
            ScriptPatternElement* element = m_elements.get(0);
            while(index >= element->getRepeatCount()) {
                elementIndex ++;
                index -= element->getRepeatCount();
                element = m_elements.get(elementIndex);
            }
            IScriptValue* val = element?element->getValue() : NULL;
            if (val == NULL || val->isNull(cmd)) {
                return defaultValue;
            }
            return val->getFloatValue(cmd,defaultValue);
  
        }
        virtual bool getBoolValue(IScriptCommand* cmd,  bool defaultValue)
        {
            // bool probably doesn't make sense for a pattern.  return true if there are elements
           return m_count > 0;
        }

        virtual DRString toString()
        {
            DRString result("pattern:");
            return result;
        }

        void setAnimator(IValueAnimator* animator) {
            m_animate = animator;
        }

        IScriptValue* eval(IScriptCommand * cmd, double defaultValue=0) override{
            auto animator = m_animate ? m_animate->clone(cmd) : NULL;
            auto clone = new PatternValue(animator);
            m_elements.each([&](ScriptPatternElement*element) {
                IScriptValue* vclone = element->getValue()->eval(cmd,defaultValue);

                clone->addElement(new ScriptPatternElement(element->getRepeatCount(),vclone));
            });
            return clone;
        }


    protected:
        PtrList<ScriptPatternElement*> m_elements;
        size_t m_count;

        IValueAnimator* m_animate;
        PatternExtend m_extend;
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
            return getFloatValue(cmd,defaultValue);
        }

        virtual double getFloatValue(IScriptCommand*cmd,  double defaultValue) override
        {
            int dv = m_hasDefaultValue ? m_defaultValue : defaultValue;
            if (m_recurse) {
                m_logger->always("variable getFloatValue() recurse");
                return dv;
            }
            m_recurse = true;
            IScriptValue * val = cmd->getValue(m_name);
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

        bool equals(IScriptCommand*cmd, const char * match) override {
            IScriptValue * val = cmd->getValue(m_name);
            return val ? val->equals(cmd,match) : false;
        }

        int getMsecValue(IScriptCommand* cmd,  int defaultValue) override { 
            IScriptValue * val = cmd->getValue(m_name);

            return val ? val->getMsecValue(cmd,defaultValue) : defaultValue;
        }

        bool isNumber(IScriptCommand* cmd) { 
            IScriptValue * val = cmd->getValue(m_name);
            return val ? val->isNumber(cmd) : false;

         }
        bool isString(IScriptCommand* cmd) { 
            IScriptValue * val = cmd->getValue(m_name);
            return val ? val->isString(cmd) : false;

         }
        bool isBool(IScriptCommand* cmd) { 
            IScriptValue * val = cmd->getValue(m_name);
            return val ? val->isBool(cmd) : false;
         }
         bool isNull(IScriptCommand* cmd) { 
            IScriptValue * val = cmd->getValue(m_name);
            return val ? val->isNull(cmd) : false;
         }

        IScriptValue* eval(IScriptCommand * cmd, double defaultValue=0) override{
            return new ScriptNumberValue(getFloatValue(cmd,defaultValue));

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
                m_logger->debug("create ScriptValueList()");
            }

            virtual ~ScriptValueList() {
                m_logger->debug("delete ~ScriptValueList()");
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

            void each(auto&& lambda) const {
                m_values.each(lambda);
            }
 
            void destroy() { delete this;}

            int count() { return m_values.size();}
        private:
            PtrList<NameValue*> m_values;
            Logger* m_logger;
   };

   IScriptValue* ScriptValue::eval(IScriptCommand * cmd, double defaultValue) {
        return new ScriptNumberValue(getFloatValue(cmd,defaultValue));
   }
}
#endif