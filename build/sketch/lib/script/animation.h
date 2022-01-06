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
        }

        double getValue(double position)
        {
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
            return value;
        }

        double getLow() { return m_low; }
        double getHigh() { return m_high; }
        double getDistance() { return (m_high-m_low) * (m_unfold ? 2 : 1);}
        void setUnfolded(bool unfold=true) { m_unfold = unfold;}
    private:
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
        }

        /* return position percent from m_low to m_high.
            if value is above m_high, use modulus
         */
        double getPosition()
        {
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
            return pct;
        }

    public:
        Logger *m_logger;
        virtual double getMin() = 0;
        virtual double getMax() = 0;
        virtual double getValue() = 0;

    };

    class TimeDomain : public AnimationDomain
    {
    public:
        // domain wraps after period
        TimeDomain() : AnimationDomain()
        {
            m_timePosition = 0;
        }

        void setTimePosition(int pos) {
            m_timePosition = pos;
        }
    public:

        virtual double getMin()
        {
            return (double)m_startMillis;
        }
        virtual double getMax()
        {
            return (double)m_endMillis;
        }
        virtual double getValue()
        {
            return (double)m_timePosition;
        }

        void setDurationMsecs(int msecs) {
            m_durationMmsecs = msecs;
            setSpan(msecs);
        }

        void setSpan(int msecs) {
            int now = m_timePosition;
            if (m_durationMmsecs == 0) {
                m_startMillis = now;
                m_endMillis=now;
                m_logger->errorNoRepeat("TimeDomain duration is 0");
                return;
            }
            int pos = (now % msecs);
            m_startMillis = now-pos;
            m_endMillis = m_startMillis+m_durationMmsecs;
            m_logger->never("TimeDomain %04X-%04X.  now=%04X",m_startMillis,m_endMillis,now);
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
            setSpan(duration);  
        }

    protected:
        int m_timePosition;
        int m_durationMmsecs;
        int m_startMillis;
        int m_endMillis;
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
            m_pos = pos;
            m_min = min;
            m_max = max;
        }
        void setPos(double p) { m_pos = p;}
        void setMin(double m) { m_min = m;}
        void setMax(double m) { m_max = m;}
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
            /* simple 
            if (position<0.5){
                return 4*pow(position,3);
            } else {
                return 1-pow(-2*position+2,3)/2;
            }
            */
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

        double get(AnimationRange &range)
        {
            if (m_ease == NULL) {
                m_ease = &DefaultEase;
            }
            m_logger->debug("Animator.get()");
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

    class ValueAnimator : public IValueAnimator
    {
    public:
        ValueAnimator()
        {
            m_logger = &AnimationLogger;
            m_repeatValue = 0;
            m_repeatDelayValue = 0;
            m_unfoldValue = NULL;
            m_unfold = false;
            m_selectedEase = &m_cubicBeszierEase;
            m_cubicBeszierEase.setValues(0,1);
            m_status = SCRIPT_CREATED;
            m_pauseUntil = 0;
            m_endDomainValue = 0;
            m_repeatCount = 0;
        }

        virtual ~ValueAnimator()
        {
            delete m_repeatValue;
            delete m_repeatDelayValue;
            delete m_unfoldValue;
        };

        void destroy() override { delete this; }

      
        void setRepeat(IScriptValue *repeat)
        {
            delete m_repeatValue;
            m_repeatValue = repeat;
        }
        void setRepeatDelay(IScriptValue *repeatDelay)
        {
            delete m_repeatDelayValue;
            m_repeatDelayValue = repeatDelay;
        }

        void setUnfold(IScriptValue *unfold)
        {
            delete m_unfoldValue;
            m_unfoldValue = unfold;
        }

        bool isUnfolded(IScriptCommand *cmd) override
        {
            return m_unfoldValue && m_unfoldValue->getBoolValue(cmd, false);
        }

        double get(IScriptCommand*cmd, AnimationRange&range) override {
            m_logger->debug("ValueAnimator.get() 0x%04X",cmd);
            AnimationDomain* domain = getDomain(cmd,range);
            setEaseParameters(cmd);
            Animator animator(*domain,m_selectedEase);
            range.setUnfolded(isUnfolded(cmd));
            double value = animator.get(range);
            return value;

/*
            if (domain == NULL) {
                m_logger->error("Animation domain is NULL");
                m_status = SCRIPT_ERROR;
                return range.getHigh();
            }
            if (m_status == SCRIPT_COMPLETE||m_status == SCRIPT_ERROR){
                return range.getHigh();
            } else if (m_status == SCRIPT_PAUSED && millis()<m_pauseUntil) {
                return range.getHigh();
            } else if (m_status == SCRIPT_CREATED||m_status == SCRIPT_PAUSED) {
                m_endDomainValue = domain->getMax();
                m_repeatCount = -1;
                if (m_repeatValue == NULL || m_repeatValue->isBool(cmd) && !m_repeatValue->getBoolValue(cmd,true)) {
                    m_repeatCount = 1;
                } else if (m_repeatValue != NULL && m_repeatValue->isNumber(cmd)) {
                    m_repeatCount = m_repeatValue->getIntValue(cmd,-1);
                }
                m_status = SCRIPT_RUNNING;
            } 
            setEaseParameters(cmd);
            Animator animator(*domain,m_selectedEase);
            range.setUnfolded(isUnfolded(cmd));
            double value = animator.get(range);
            if (domain->getValue() > m_endDomainValue) {
                m_logger->always("paused");
                if (m_repeatCount>0) {
                    m_repeatCount--;
                }
                if (m_repeatCount == 0) {
                    m_logger->always("complete");
                    m_status = SCRIPT_COMPLETE;
                } else {
                    int pauseMsecs = m_repeatDelayValue ? m_repeatDelayValue->getMsecValue(cmd,0) : 0;
                    if (pauseMsecs > 0) {
                        m_status= SCRIPT_PAUSED;
                        m_pauseUntil = millis()+pauseMsecs;
                    }
                }
            }
            return value;
*/            
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
        IScriptValue *m_repeatValue;
        IScriptValue *m_repeatDelayValue;
        IScriptValue *m_unfoldValue;
        IScriptValue *m_ease;
        IScriptValue *m_easeIn;
        IScriptValue *m_easeOut;
        CubicBezierEase m_cubicBeszierEase;
        LinearEase m_linearEase;
        AnimationEase* m_selectedEase;
        ScriptStatus  m_status;
        int m_endDomainValue;
        int m_pauseUntil;
        bool m_unfold;
        int m_repeatCount;
        Logger *m_logger;
    };

    class SpeedValueAnimator : public ValueAnimator {
        public:
            SpeedValueAnimator(IScriptValue* speed) { m_speedValue = speed;}

            virtual ~SpeedValueAnimator(){

            }

            AnimationDomain* getDomain(IScriptCommand*cmd,AnimationRange&range) override {
                TimeDomain* domain = cmd->getState()->getAnimationTimeDomain();

                if (domain == NULL) {
                    m_logger->errorNoRepeat("ValueAnimation getSpeedValue requires a TimeDomain from the state");
                    return NULL;
                }
                int speed = m_speedValue->getIntValue(cmd,1);
                domain->setSpeed(speed,&range);
                return domain;
            }

            void setSpeed(IScriptValue* speed) { m_speedValue = speed;}
        private: 
            IScriptValue *m_speedValue;

    };

    class DurationValueAnimator : public ValueAnimator {
        public:
            DurationValueAnimator(IScriptValue* duration) { m_durationValue = duration;}

            virtual ~DurationValueAnimator(){

            }
            AnimationDomain* getDomain(IScriptCommand*cmd,AnimationRange&range) override {
                TimeDomain* domain = cmd->getState()->getAnimationTimeDomain();

                if (domain == NULL) {
                    return NULL;
                }
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
                PositionDomain* domain = cmd->getState()->getAnimationPositionDomain();
                return domain;
            }

        private: 

    };

}

#endif