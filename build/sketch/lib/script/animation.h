#line 1 "d:\\dev\\arduino\\led_app\\basic_controller\\lib\\script\\animation.h"
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
        bool getUnfold() { return m_unfold;}
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
            m_logger->never("create AnimationDomain");
        }

        // update based on current state if implementation needs to
        virtual void update(IScriptState* state) {}


        /* return position percent from m_low to m_high.
            if value is above m_high, use modulus
         */
        double getPosition()
        {
            m_logger->never("AnimationDomain.getPosition");
            if (!m_changed) { 
                m_logger->never("\tno change");
                return m_lastValue;
            }
            m_logger->debug("getPosition()");
            double value = getValue();
            double low = getMin();
            double high = getMax();

            if (value <= low)
            {
                m_logger->never("\t low %f<%f",value,low);
                return 0;
            }
            if (value >= high)
            {
                m_logger->never("\thigh %f>%f",value,high);
                return 1;
            }
            double diff = high - low;
            double pct = diff == 0 ? 1 : (value - low) / (diff);
            m_logger->never("\thigh=%f low=%f diff=%f value=%f pos=%f",high,low,diff,value,pct);
            m_changed = false;
            m_lastValue = pct;
            return pct;
        }

        bool isChanged() { return m_changed;}
        double getSpan() {
            return abs(getMax()-getMin());
        }
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
            m_lastStep = -1;
        }

        void update(IScriptState* state) override {
            if (m_lastStep == state->getStepNumber()) {
                m_logger->never("time domain no change %d",m_lastStep);

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
                m_logger->never("time domain duration==0");

            } else {
                int diff = m_val - m_startMillis;
                m_repeat = diff / m_durationMmsecs;
                m_min = m_startMillis + m_repeat*m_durationMmsecs;
                m_max = m_min + m_durationMmsecs;
                m_logger->never("time domain %d %d %d %d %d",m_val,m_startMillis,m_min,m_max,m_lastStep);
            }
        }

        virtual void setStart(int msecs = -1) {
            m_startMillis = (msecs == -1) ? millis() : msecs;
            m_repeat=0;
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

        virtual int getRepeatCount() { 
            return m_repeat;
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
            m_logger->never("\tpos %f.  ease %f. result %f.  ",result);
            return result;
        };

        void setEase(AnimationEase* ease) { m_ease = ease;}
    private:
        AnimationDomain &m_domain;
        AnimationEase* m_ease;
        Logger *m_logger;
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
            m_isComplete = false;
            m_ease = NULL;
            m_easeIn = NULL;
            m_easeOut = NULL;
        }

        ValueAnimator(ValueAnimator*other, IScriptCommand*cmd)
        {
            m_logger = &AnimationLogger;
            m_unfoldValue = other->m_unfoldValue ? other->m_unfoldValue->eval(cmd,false) : NULL;
            m_ease = other->m_ease ? other->m_ease->eval(cmd,false) : NULL;
            m_easeIn = other->m_easeIn ? other->m_easeIn->eval(cmd,false) : NULL;
            m_easeOut = other->m_easeOut ? other->m_easeOut->eval(cmd,false) : NULL;
            m_unfold = false;
            m_selectedEase = other->m_selectedEase;
            m_cubicBeszierEase.setValues(0,1);
            m_isComplete = false;

        }

        virtual ~ValueAnimator()
        {
            if (m_unfoldValue) {m_unfoldValue->destroy();}
            if (m_ease) {m_ease->destroy();}
            if (m_easeIn) {m_easeIn->destroy();}
            if (m_easeOut) {m_easeOut->destroy();}
        };

        void destroy() override { delete this; }

        bool isComplete() { return m_isComplete;}

        void setUnfold(IScriptValue *unfold)
        {
            if (m_unfoldValue) {m_unfoldValue->destroy();}
            m_unfoldValue = unfold;
        }

        bool isUnfolded(IScriptCommand *cmd) override
        {
            return m_unfoldValue && m_unfoldValue->getBoolValue(cmd, false);
        }

     
        double get(IScriptCommand*cmd, AnimationRange&range) override {
            m_logger->debug("ValueAnimator.get() 0x%04X",cmd);


            m_logger->debug("\tcheck paused");

            m_logger->debug("\tget domain");
            AnimationDomain* domain = getDomain(cmd,range);
            m_logger->debug("\tset ease");
            setEaseParameters(cmd);
            if (domain == NULL) {
                m_logger->error("Animation domain is NULL");
                return range.getHigh();
            }
            m_logger->debug("\tcreate animator");

            Animator animator(*domain,m_selectedEase);
            m_logger->debug("\tset unfold");
            range.setUnfolded(isUnfolded(cmd));
            m_logger->debug("\tget value");

            if (isPaused(cmd,range)) {
                m_logger->debug("\treturn pause value");
                return getPauseValue(cmd,range);
            }
            double value = animator.get(range,cmd);
            m_lastValue = value;
            m_logger->never("ValueAnimator value=%f",value);

            return value;
          
        }

        virtual AnimationDomain* getDomain(IScriptCommand*cmd,AnimationRange&range) =0;

        void setEaseParameters(IScriptCommand* cmd) {
            double in = 1;
            double out = 1;
            if ((m_ease != NULL && m_ease->equals(cmd,"linear"))){
                m_selectedEase = &m_linearEase;
                return;
            } else {
                m_selectedEase = &m_cubicBeszierEase;
            }
            if (m_ease) {
                in = 1-m_ease->getFloatValue(cmd,1);
                out = 1-in;
                m_logger->never("got ease %x %f %f",this,in,out);
            }
            if (m_easeIn) {
                in = 1-m_easeIn->getFloatValue(cmd,0.123);
                m_logger->never("got ease-in %x %f",this,in);

            }

            if (m_easeOut) {
                out = m_easeOut->getFloatValue(cmd,0);
                m_logger->never("got ease-out %x %f",this,out);
            }
            m_logger->never("ease in/out  %f/%f",in,out);
            m_cubicBeszierEase.setValues(in,out);
        }


        void setEase(IScriptValue* ease) { 
            m_logger->never("ease %x %x %s",this,ease, (ease ? ease->toString().text():""));
            m_ease = ease;
        }

        void setEaseIn(IScriptValue* ease) {
            m_logger->never("ease-in %x %x %s",this,ease, (ease ? ease->toString().text():""));
            m_easeIn = ease;
        }
        void setEaseOut(IScriptValue* ease) {
            m_logger->never("ease-out %x %x %s",this,ease, (ease ? ease->toString().text():""));
            m_easeOut = ease;
        }

    protected:
        virtual bool isPaused(IScriptCommand* cmd, AnimationRange&range) { return false;}
        virtual double getPauseValue(IScriptCommand* cmd, AnimationRange&range) { return m_lastValue;}
        double m_lastValue;
        IScriptValue *m_unfoldValue;
        IScriptValue *m_ease;
        IScriptValue *m_easeIn;
        IScriptValue *m_easeOut;
        CubicBezierEase m_cubicBeszierEase;
        LinearEase m_linearEase;
        AnimationEase* m_selectedEase;
        bool m_unfold;
        bool m_isComplete;


        Logger *m_logger;
    };


    class TimeValueAnimator : public ValueAnimator {
        public:
            TimeValueAnimator() {
                m_repeatValue = NULL;
                m_delayValue = NULL;
                m_delayResponseValue = NULL;
                m_iterationCount = 0;
                m_delayUntil = 0;
            }

            TimeValueAnimator(TimeValueAnimator* other,IScriptCommand*cmd) : ValueAnimator(other,cmd) {
                m_repeatValue = other->m_repeatValue ? other->m_repeatValue->eval(cmd,0) : NULL;
                m_delayValue = other->m_delayValue ? other->m_delayValue->eval(cmd,0) : NULL;
                m_delayResponseValue = other->m_delayResponseValue ? other->m_delayResponseValue->eval(cmd,0) : NULL;
                m_iterationCount = 0;
                m_delayUntil = 0;
                m_timeDomain.setStart();
            }

            virtual  ~TimeValueAnimator(){
                if (m_repeatValue) {m_repeatValue->destroy();}
                if (m_delayValue) {m_delayValue->destroy();}
                if (m_delayResponseValue) {m_delayResponseValue->destroy();}
            }

            void setRepeatValue(IScriptValue*repeat) { 
                if (m_repeatValue) {m_repeatValue->destroy();}
                    m_repeatValue = repeat;
                }
            void setRepeatDelayValue(IScriptValue*delay) { 
                if (m_delayValue) {m_delayValue->destroy();}
                m_delayValue = delay;
            }
            void setDelayResponseValue(IScriptValue*delayResponse) { 
                if (m_delayResponseValue) {m_delayResponseValue->destroy();}
                m_delayResponseValue = delayResponse;
            }

        protected: 
            bool isPaused(IScriptCommand* cmd, AnimationRange&range) override  { 
                m_timeDomain.update(cmd->getState());
                m_logger->debug("check repeat count");
                if (m_timeDomain.getRepeatCount()==0) {
                    m_logger->debug("\tno repeat");
                    return false;
                };
                m_logger->debug("\tgot repeat",m_timeDomain.getRepeatCount());
                if (m_delayUntil == 0) {
                    m_logger->debug("\tincrement iteration %d",m_iterationCount);
                    
                    m_iterationCount += 1;
                    if (m_repeatValue) {
                        m_logger->debug("\tcheck max repeat");

                        int maxRepeat = m_repeatValue->getIntValue(cmd,1);
                        if (m_iterationCount>= maxRepeat) {
                            m_logger->debug("\treached max");
                            m_isComplete = true;
                            cmd->onAnimationComplete(this);
                            return true;
                        }
                    }

                    int delayMsecs = m_delayValue == NULL ? 1 : m_delayValue->getIntValue(cmd,1);
                    m_delayUntil = millis() + delayMsecs;
                    return true;
                } else if (m_delayValue != NULL) {
                    if (millis() > m_delayUntil) {
                        m_delayUntil = 0;
                        m_timeDomain.setStart();
                        return false;
                    } else {
                        return true;
                    }
                } else {
                    m_delayUntil = 0;
                    m_timeDomain.setStart();
                    return false;
                }
                return false;
            }
            
            double getPauseValue(IScriptCommand* cmd, AnimationRange&range) override {
                if (m_delayResponseValue != NULL) {
                    double val = m_delayResponseValue->getFloatValue(cmd,m_lastValue);
                    m_logger->never("return response value %f (high=%f)",val,range.getHigh());
                    return val;
                }
                m_logger->never("return high value %f",range.getHigh());
                return range.getUnfold() ? range.getLow() :  range.getHigh();
            }



            int m_iterationCount;
            int m_delayUntil;
            IScriptValue *m_repeatValue;
            IScriptValue *m_delayValue;
            IScriptValue *m_delayResponseValue;
            TimeDomain  m_timeDomain;
    };


    class SpeedValueAnimator : public TimeValueAnimator {
        public:
            SpeedValueAnimator(IScriptValue* speed) { m_speedValue = speed;}
            SpeedValueAnimator(SpeedValueAnimator* other,IScriptCommand*cmd) : TimeValueAnimator(other,cmd) {
                 m_speedValue = other->m_speedValue->eval(cmd,0);
            }

            virtual ~SpeedValueAnimator(){

            }

            AnimationDomain* getDomain(IScriptCommand*cmd,AnimationRange&range) override {
                TimeDomain* domain = &m_timeDomain;

                int speed = m_speedValue->getIntValue(cmd,1);
                domain->setSpeed(speed,&range);
                return domain;
            }

            void setSpeed(IScriptValue* speed) { m_speedValue = speed;}
            
            IValueAnimator* clone(IScriptCommand* cmd) {
                SpeedValueAnimator* other = new SpeedValueAnimator(this,cmd); //m_speedValue->eval(cmd,0));
                return other;
            }

        private: 
            IScriptValue *m_speedValue;

    };
    class DurationValueAnimator : public TimeValueAnimator {
        public:
            DurationValueAnimator(IScriptValue* duration) { 
                m_durationValue = duration;
            }

            DurationValueAnimator(DurationValueAnimator* other,IScriptCommand*cmd) : TimeValueAnimator(other,cmd) {
                 m_durationValue = other->m_durationValue->eval(cmd,0);

            }
            virtual ~DurationValueAnimator(){
                if (m_durationValue) {m_durationValue->destroy();}
            }
            AnimationDomain* getDomain(IScriptCommand*cmd,AnimationRange&range) override {
                TimeDomain* domain = &m_timeDomain;

                int duration = m_durationValue ? m_durationValue->getIntValue(cmd,1000) : 1000;
                domain->setDuration(duration);
                return domain;
            }

            void setDuration(IScriptValue* duration) { m_durationValue = duration;}

            IValueAnimator* clone(IScriptCommand* cmd) {
                m_logger->never("clone duration %f",m_durationValue->getFloatValue(cmd,-2));
                DurationValueAnimator* other = new DurationValueAnimator(this,cmd); //m_speedValue->eval(cmd,0));
                m_logger->never("\tcloned duration %f",other->m_durationValue->getFloatValue(cmd,-2));
                return other;
            }

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

            IValueAnimator* clone(IScriptCommand* cmd) {
                return new PositionValueAnimator();
            }

        private: 

    };

}

#endif