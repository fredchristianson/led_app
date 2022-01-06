#ifndef DR_ANIMATION_H
#define DR_ANIMATION_H

#include "../logger.h"
#include "./script_interface.h"

namespace DevRelief
{
    Logger AnimationLogger("Animation", ANIMATION_LOGGER_LEVEL);

  class AnimationRange
    {
    public:
        AnimationRange(double low, double high, bool unfold=false)
        {
            m_low = low;
            m_high = high;
            m_unfold = unfold;
            m_logger = &AnimationLogger;
            m_logger->debug("create AnimationRange %f-%f  %s",low,high,unfold?"unfold":"");
            m_lastPosition = 99999999;
            m_lastValue = 0;
        }

        double getValue(double position)
        {
            if (position == m_lastPosition) { return m_lastValue;}
            m_logger->never("AnimationRange.getValue(%f)  %f-%f",position,m_low,m_high);
            if (m_unfold) {
                if (position<=0.5) {
                    position=position*2;
                } else {
                    position= (1-position)*2;
                }
            }
            if (position <= 0 || m_high == m_low)
            {
                m_logger->never("\t return low %f",position,m_low);

                return m_low;
            }
            if (position >= 1)
            {
                m_logger->never("\treturn high %f",position,m_high);
                return m_high;
            }
            double diff = m_high - m_low;
            double value = m_low + position * diff;
            m_logger->never("\t %f  %f %f-%f",value,diff,m_low,m_high);
            m_lastPosition = position;
            m_lastValue = value;
            return value;
        }

        double getLow() { return m_low; }
        double getHigh() { return m_high; }
        double getDistance() { return (m_high-m_low) * (m_unfold ? 2 : 1);}
        void setUnfolded(bool unfold=true) { m_unfold = unfold;}
    private:
        double m_lastPosition;
        double m_lastValue;
        double m_low;
        double m_high;
        bool m_unfold;
        Logger* m_logger;
    };


    class AnimationDomain
    {
    public:
        AnimationDomain()
        {
            m_logger = &AnimationLogger;
            m_changed = true;
        }

        // update based on current state if implementation needs to
        virtual void update(IScriptState* state) {}


        /* return position percent from m_low to m_high.
            if value is above m_high, use modulus
         */
        double getPosition()
        {
            if (!m_changed) { return m_lastValue;}
            m_logger->debug("getPosition()");
            double value = getValue();
            double low = getMin();
            double high = getMax();

            if (value <= low)
            {
                m_logger->debug("%f<%f",value,low);
                return 0;
            }
            if (value >= high)
            {
                m_logger->debug("%f>%f",value,high);
                return 1;
            }
            double diff = high - low;
            double pct = diff == 0 ? 1 : (value - low) / (diff);
            m_logger->debug("\thigh=%f low=% diff=% value=% pos=%f",high,low,diff,value,pct);
            m_changed = false;
            m_lastValue = pct;
            return pct;
        }

        bool isChanged() { return m_changed;}

    public:
        Logger *m_logger;
        virtual double getMin() = 0;
        virtual double getMax() = 0;
        virtual double getValue() = 0;
        
    protected:
        bool m_changed;
        double m_lastValue;

    };

    class TimeDomain : public AnimationDomain
    {
    public:
        // domain wraps after period
        TimeDomain() : AnimationDomain()
        {
            m_durationMmsecs = 0;
            m_startMillis = millis();
            m_min = m_startMillis;
            m_max = m_startMillis;
            m_val  = m_startMillis;
            m_repeat = 0;
        }

        void update(IScriptState* state) override {
            if (m_lastStep == state->getStepNumber()) {
                return;
            }
            m_changed = true;
            m_val = state->getStepStartTime();
            m_lastStep = state->getStepNumber();
            if (m_durationMmsecs == 0) {
                m_startMillis = m_val;
                m_min = m_startMillis;
                m_max = m_startMillis;
                m_val  = m_startMillis;
            } else {
                m_val = m_val;
                int diff = m_val - m_startMillis;
                m_repeat = diff / m_durationMmsecs;
                m_min = m_startMillis + m_repeat*m_durationMmsecs;
                m_max = m_min + m_durationMmsecs;
            }
        }

        virtual void setStart(int msecs = -1) {
            m_startMillis = (msecs == -1) ? millis() : msecs;
        }

        virtual double getMin()
        {
            return m_min;
        }
        virtual double getMax()
        {
            return m_max;
        }
        virtual double getValue()
        {
            return m_val;
        }

        void setDurationMsecs(int msecs) {
            m_durationMmsecs = msecs;
        }
    

        void setSpeed(double speed, AnimationRange* range) {
            if (speed == 0) { 
                setDurationMsecs(0);
            } else {
                double distance = range->getDistance();
                int duration = 1000*distance/speed;
                setDurationMsecs(duration);
            }            
        }

        void setDuration(double duration) {
            m_durationMmsecs = duration;
        }

    protected:
        int m_lastStep;
        int m_durationMmsecs;
        int m_startMillis;
        int m_min;
        int m_max;
        int m_val;
        int m_repeat;
    };

   

    class PositionDomain : public AnimationDomain
    {
    public:
        PositionDomain()
        {
        }

    public:
        double getValue() override
        {
            return (double)m_pos;
        }
        double getMax() override
        {
            return (double)m_max;
        }
        double getMin() override
        {
            return m_min;
        }

        void setPosition(double pos, double min, double max) {
            m_changed = true;
            m_pos = pos;
            m_min = min;
            m_max = max;
        }
        void setPos(double p) { m_pos = p;m_changed = true;}
        void setMin(double m) { m_min = m;m_changed = true;}
        void setMax(double m) { m_max = m;m_changed = true;}
    private:
        double m_min;        
        double m_max;
        double m_pos;
    };

    class StepDomain : public AnimationDomain
    {
    };

    

    class AnimationEase
    {
    public:
        AnimationEase() {
            m_logger = &AnimationLogger;
        }
        virtual double calculate(double position) = 0;

    protected:
        Logger* m_logger;
    };

    class LinearEase : public AnimationEase
    {
    public:
        double calculate(double position)
        {
            m_logger->debug("Linear ease %f",position);
            return position;
        }
    };

    LinearEase DefaultEase;

    class CubicBezierEase : public AnimationEase
    {
    public:
        CubicBezierEase(double in=0.65, double out=0.35){
            m_in = in;
            m_out = out;
        }

        void setValues(double in, double out) {
            m_in = in;
            m_out = out;
        }
        double calculate(double position)
        {

            double val = 3 * pow(1 - position, 2) * position * m_in +
                            3 * (1 - position) * pow(position, 2) * m_out +
                            pow(position, 3);
            AnimationLogger.never("ease: %f==>%f  %f  %f",position,val,m_in,m_out);
            return val;
        }
    private:
        double m_in;
        double m_out;
        
    };


    class Animator
    {
    public:
        Animator(AnimationDomain &domain, AnimationEase *ease = &DefaultEase) : m_domain(domain), m_ease(ease)
        {
            m_logger = &AnimationLogger;
            m_logger->debug("create Animator()");
        }

        double get(AnimationRange &range, IScriptCommand* cmd)
        {
            if (m_ease == NULL) {
                m_ease = &DefaultEase;
            }
            m_logger->debug("Animator.get()");
            m_domain.update(cmd->getState());
            double position = m_domain.getPosition();
            double ease = m_ease->calculate(position);
            //m_logger->debug("\tpos %f.  ease %f",position,ease);
            double result = range.getValue(ease);
            //m_logger->debug("\tpos %f.  ease %f. result %f.  ",result);
            return result;
        };

        void setEase(AnimationEase* ease) { m_ease = ease;}
    private:
        AnimationDomain &m_domain;
        AnimationEase* m_ease;
        Logger *m_logger;
    };

    class AnimatorState : public IScriptValue {
        public:
        AnimatorState() {
            m_status= SCRIPT_RUNNING;
            m_pauseUntil = 0;
        }

        void destroy() {delete this;}

        ScriptStatus  m_status;
        int m_endDomainValue;
        int m_pauseUntil;
        virtual int getRepeatCount() { return 0;}

        

        // these methods are required but not implemented for this IScriptValue
        int getIntValue(IScriptCommand* cmd,  int defaultValue) { return defaultValue;}
        double getFloatValue(IScriptCommand* cmd,  double defaultValue)  { return defaultValue;}
        bool getBoolValue(IScriptCommand* cmd,  bool defaultValue)  { return defaultValue;}
        int getMsecValue(IScriptCommand* cmd,  int defaultValue)  { return defaultValue;}

        bool isString(IScriptCommand* cmd) { return false;}
        bool isNumber(IScriptCommand* cmd) { return false;}
        bool isBool(IScriptCommand* cmd) { return false;}

        bool isRecursing()  { return false;}
        // for debugging
        DRString toString() { return DRFormattedString("AnimatorState %d %d",m_status,m_pauseUntil);};
        bool equals(IScriptCommand*cmd, const char * match) { return false;}
    };

    class ValueAnimator : public IValueAnimator
    {
    public:
        ValueAnimator()
        {
            m_logger = &AnimationLogger;
            m_unfoldValue = NULL;
            m_unfold = false;
            m_selectedEase = &m_cubicBeszierEase;
            m_cubicBeszierEase.setValues(0,1);

        }

        virtual ~ValueAnimator()
        {
            delete m_unfoldValue;
        };

        void destroy() override { delete this; }

    

        void setUnfold(IScriptValue *unfold)
        {
            delete m_unfoldValue;
            m_unfoldValue = unfold;
        }

        bool isUnfolded(IScriptCommand *cmd) override
        {
            return m_unfoldValue && m_unfoldValue->getBoolValue(cmd, false);
        }

        AnimatorState* getState(IScriptCommand* cmd) {
            AnimatorState* as = (AnimatorState*)cmd->getState()->getValue(this,"animator-state");
            if (as == NULL) {
                as = new AnimatorState();
                cmd->getState()->setValue(this,"animator-state",as);
            }
            return as;
        }

        double get(IScriptCommand*cmd, AnimationRange&range) override {
            m_logger->debug("ValueAnimator.get() 0x%04X",cmd);

            AnimatorState* as = getState(cmd);
            AnimationDomain* domain = getDomain(cmd,range);
            setEaseParameters(cmd);
            if (domain == NULL) {
                m_logger->error("Animation domain is NULL");
                as->m_status = SCRIPT_ERROR;
                return range.getHigh();
            }

            Animator animator(*domain,m_selectedEase);
            range.setUnfolded(isUnfolded(cmd));
            double value = animator.get(range,cmd);
           
            return value;
          
        }

        virtual AnimationDomain* getDomain(IScriptCommand*cmd,AnimationRange&range) =0;

        void setEaseParameters(IScriptCommand* cmd) {
            double in = 1;
            double out = 1;
            if (m_ease == NULL || (m_ease->equals(cmd,"linear"))){
                m_selectedEase = &m_linearEase;
            }
            if (m_ease) {
                in = 1-m_ease->getFloatValue(cmd,1);
                out = 1-in;
            }
            if (m_easeIn) {
                in = 1-m_easeIn->getFloatValue(cmd,0);
            }

            if (m_easeOut) {
                out = m_easeOut->getFloatValue(cmd,0);
            }
            m_cubicBeszierEase.setValues(in,out);
        }


        void setEase(IScriptValue* ease) { m_ease = ease;}
        void setEaseIn(IScriptValue* ease) { m_easeIn = ease;}
        void setEaseOut(IScriptValue* ease) { m_easeOut = ease;}

    protected:
        IScriptValue *m_durationValue; // ignored if m_seed is set

        IScriptValue *m_unfoldValue;
        IScriptValue *m_ease;
        IScriptValue *m_easeIn;
        IScriptValue *m_easeOut;
        CubicBezierEase m_cubicBeszierEase;
        LinearEase m_linearEase;
        AnimationEase* m_selectedEase;
        bool m_unfold;


        Logger *m_logger;
    };


    class TimeValueAnimator : public ValueAnimator {
        public:
            TimeValueAnimator() {
                m_repeatValue = NULL;
                m_delayValue = NULL;
                m_delayResponseValue = NULL;
            }

            virtual  ~TimeValueAnimator(){
                delete m_repeatValue;
                delete m_delayValue;
                delete m_delayResponseValue;
            }

            void setRepeatValue(IScriptValue*repeat) { 
                delete m_repeatValue;
                m_repeatValue = repeat;
                }
            void setRepeatDelayValue(IScriptValue*delay) { 
                delete m_delayValue;
                m_delayValue = delay;
            }
            void setDelayResponseValue(IScriptValue*delayResponse) { 
                delete m_delayResponseValue;
                m_delayResponseValue = delayResponse;
            }

        protected: 
            IScriptValue *m_repeatValue;
            IScriptValue *m_delayValue;
            IScriptValue *m_delayResponseValue;
            TimeDomain  m_timeDomain;
    };


    class SpeedValueAnimator : public TimeValueAnimator {
        public:
            SpeedValueAnimator(IScriptValue* speed) { m_speedValue = speed;}

            virtual ~SpeedValueAnimator(){

            }

            AnimationDomain* getDomain(IScriptCommand*cmd,AnimationRange&range) override {
                TimeDomain* domain = &m_timeDomain;

                int speed = m_speedValue->getIntValue(cmd,1);
                domain->setSpeed(speed,&range);
                return domain;
            }

            void setSpeed(IScriptValue* speed) { m_speedValue = speed;}
        private: 
            IScriptValue *m_speedValue;

    };
    class DurationValueAnimator : public TimeValueAnimator {
        public:
            DurationValueAnimator(IScriptValue* duration) { m_durationValue = duration;}

            virtual ~DurationValueAnimator(){

            }
            AnimationDomain* getDomain(IScriptCommand*cmd,AnimationRange&range) override {
                TimeDomain* domain = &m_timeDomain;

                int duration = m_durationValue->getIntValue(cmd,1000);
                domain->setDuration(duration);
                return domain;
            }

            void setDuration(IScriptValue* duration) { m_durationValue = duration;}

        private: 
            IScriptValue *m_durationValue;

    };

    class PositionValueAnimator : public ValueAnimator {
        public:
            PositionValueAnimator() {

            }

            virtual ~PositionValueAnimator(){

            }

            AnimationDomain* getDomain(IScriptCommand*cmd,  AnimationRange&range) override {
                PositionDomain* domain = cmd->getAnimationPositionDomain();
                return domain;
            }

        private: 

    };

}

#endif