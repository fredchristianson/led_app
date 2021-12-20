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
            m_logger->always("AnimationRange.getValue(%f)  %f-%f",position,m_low,m_high);
            if (m_unfold) {
                if (position<=0.5) {
                    position=position*2;
                } else {
                    position= (1-position)*2;
                }
            }
            if (position <= 0 || m_high == m_low)
            {
                m_logger->always("\t return low %f",position,m_low);

                return m_low;
            }
            if (position >= 1)
            {
                m_logger->always("\treturn high %f",position,m_high);
                return m_high;
            }
            double diff = m_high - m_low;
            double value = m_low + position * diff;
            m_logger->always("\t %f  %f %f-%f",value,diff,m_low,m_high);
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

    class Animator
    {
    public:
        Animator(AnimationDomain &domain, AnimationEase &ease = DefaultEase) : m_domain(domain), m_ease(ease)
        {
            m_logger = &AnimationLogger;
            m_logger->debug("create Animator()");
        }

        double get(AnimationRange &range)
        {
            m_logger->debug("Animator.get()");
            double position = m_domain.getPosition();
            double ease = m_ease.calculate(position);
            //m_logger->debug("\tpos %f.  ease %f",position,ease);
            double result = range.getValue(ease);
            //m_logger->debug("\tpos %f.  ease %f. result %f.  ",result);
            return result;
        };

    private:
        AnimationDomain &m_domain;
        AnimationEase &m_ease;
        Logger *m_logger;
    };

    class ValueAnimator : public IValueAnimator
    {
    public:
        ValueAnimator()
        {
            m_logger = &AnimationLogger;
            m_speedValue = 0;
            m_durationValue = 0;
            m_repeatValue = 0;
            m_repeatDelayValue = 0;
            m_unfoldValue = NULL;
            m_unfold = false;
        }

        virtual ~ValueAnimator()
        {
            delete m_speedValue;
            delete m_durationValue;
            delete m_repeatValue;
            delete m_repeatDelayValue;
            delete m_unfoldValue;
        };

        void destroy() override { delete this; }

        void setSpeed(IScriptValue *speed)
        {
            delete m_speedValue;
            m_speedValue = speed;
        }
        void setDuration(IScriptValue *duration)
        {
            delete m_durationValue;
            m_durationValue = duration;
        }
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

        double getSpeed(IScriptCommand *cmd) 
        {
            return m_speedValue ? m_speedValue->getFloatValue(cmd, 0) : 0;
        }

        double getDuration(IScriptCommand *cmd) 
        {
            return m_durationValue ? m_durationValue->getFloatValue(cmd, 0) : 0;
        }

        double get(IScriptCommand*cmd, AnimationRange&range) override {
            m_logger->debug("ValueAnimator.get() 0x%04X",cmd);
            double speed = getSpeed(cmd);
            double duration = getDuration(cmd);
            double result=range.getLow();
            m_logger->debug("\tspeed %f, duration %f",speed,duration);
            
            if (speed>0) {
                result = getSpeedValue(cmd,speed,range);
            } else if (duration>0) {
                result = getDurationValue(cmd,duration,range);
            } else {
                result = getPositionValue(cmd,range);
            }
            return result;
        }

        double getPositionValue(IScriptCommand*cmd, AnimationRange&range){
            PositionDomain* domain = cmd->getState()->getAnimationPositionDomain();
            if (domain == NULL) {
                m_logger->errorNoRepeat("ValueAnimation getPositionValue requires a PositionDomain from the state");
                return range.getLow();
            }
            Animator animator(*domain);
            range.setUnfolded(isUnfolded(cmd));
            double value = animator.get(range);
            return value;
        }

        double getSpeedValue(IScriptCommand*cmd, double speed, AnimationRange&range){
            range.setUnfolded(isUnfolded(cmd));
            TimeDomain* domain = cmd->getState()->getAnimationTimeDomain();
             if (domain == NULL) {
                m_logger->errorNoRepeat("ValueAnimation getSpeedValue requires a TimeDomain from the state");
                return range.getLow();
            }
            domain->setSpeed(speed,&range);
            Animator animator(*domain);
            double value = animator.get(range);
            return value;
        }

        double getDurationValue(IScriptCommand*cmd, double duration, AnimationRange&range){
            range.setUnfolded(isUnfolded(cmd));
            TimeDomain* domain = cmd->getState()->getAnimationTimeDomain();
             if (domain == NULL) {
                m_logger->errorNoRepeat("ValueAnimation getSpeedValue requires a TimeDomain from the state");
                return range.getLow();
            }
            domain->setDuration(duration);
            Animator animator(*domain);
            double value = animator.get(range);
            return value;
        }

        /*----------------------------------------old*/

        double getTimeValue(IScriptState &state, double low, double high)
        {
            /*
            m_logger->test("ValueAnimator::getTimeValue %f %f",low,high);
            double stepsPerSecond = 0;
            if (low == high) {
                m_logger->test("\tvalues equal");
                return low;
            }
            double range = high-low+1;
            int durationMsecs = 0;
            if (m_speedValue != NULL) {
                double speed = m_speedValue->getFloatValue(state,0);
                stepsPerSecond = speed;
                durationMsecs =  (int)((range/stepsPerSecond)*1000);
            } else if (m_durationValue != NULL) {
                durationMsecs = m_durationValue->getFloatValue(state,0);
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
                unfold = m_unfoldValue->getBoolValue(state,false);
            }
            Animator animator(0,durationMsecs, unfold);
            double value = animator.getValueAt(low,high,timePosition);
            m_logger->debug("\ttime animator value %f",value);
            return value;
            */
            return 0;
        }

        double getPositionValue(IScriptState &state, double low, double high, double percent)
        {
            /*
            m_logger->debug("getPositionValue %f %f %f",low,high,percent);
            Animator animate(low,high,isUnfolded(state));
            double val = animate.getValueAtPercent(low,high,percent);
            m_logger->debug("\tposition animator value %f",val);
            return val;
            */
            return 0;
        }

 

        bool hasSpeedOrDuration(IScriptCommand *cmd)
        {
            return (m_speedValue && m_speedValue->getIntValue(cmd, 0) > 0) ||
                   (m_durationValue && m_durationValue->getIntValue(cmd, 0) > 0);
        }

        bool canAnimateOverTime(IScriptCommand *cmd)
        {
            m_logger->test("canAnimateOverTime");
            bool speedOrDuration = hasSpeedOrDuration(cmd);
            m_logger->test("\t%s", speedOrDuration ? "yes" : "no");
            return speedOrDuration;
        }


    protected:
        IScriptValue *m_speedValue;
        IScriptValue *m_durationValue; // ignored if m_seed is set
        IScriptValue *m_repeatValue;
        IScriptValue *m_repeatDelayValue;
        IScriptValue *m_unfoldValue;
        bool m_unfold;
        Logger *m_logger;
    };

}

#endif