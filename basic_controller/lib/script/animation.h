#ifndef DR_ANIMATION_H
#define DR_ANIMATION_H

#include "../logger.h"
#include "./script_interface.h"
#include "./script_state.h"

namespace DevRelief
{
    Logger AnimationLogger("Animation", ANIMATION_LOGGER_LEVEL);

    class AnimationDomain
    {
    public:
        AnimationDomain(ScriptState *state)
        {
            m_logger = &AnimationLogger;
            m_state = state;
        }

        /* return position percent from m_low to m_high.
            if value is above m_high, use modulus
         */
        double getPosition()
        {
            m_logger->never("getPosition()");
            double value = getValue();
            double low = getMin();
            double high = getMax();

            if (value <= low)
            {
                m_logger->never("%f<%f",value,low);
                return 0;
            }
            if (value >= high)
            {
                m_logger->never("%f>%f",value,high);
                return 1;
            }
            double diff = high - low;
            double pct = diff == 0 ? 1 : (value - low) / (diff);
            m_logger->never("\thigh=%f low=% diff=% value=% pos=%f",high,low,diff,value,pct);
            return pct;
        }

    public:
        Logger *m_logger;
        virtual double getMin() = 0;
        virtual double getMax() = 0;
        virtual double getValue() = 0;

        ScriptState *m_state;
    };

    class TimeDomain : public AnimationDomain
    {
    public:
        // domain wraps after period
        TimeDomain(ScriptState *state, double period) : AnimationDomain(state)
        {
        }

    public:
        virtual double getMin()
        {
            return m_state->getLedPosition();
        }
        virtual double getMax()
        {
            return m_state->getMaxLedPosition();
        }
        virtual double getValue()
        {
            return m_state->getMinLedPosition();
        }
    };

    class PositionDomain : public AnimationDomain
    {
    public:
        PositionDomain(ScriptState *state) : AnimationDomain(state)
        {
            m_logger->never("PositionDomain() 0x%04X", state);
        }

    public:
        double getValue() override
        {
            return (double)m_state->getLedPosition();
        }
        double getMax() override
        {
            return (double)m_state->getMaxLedPosition();
        }
        double getMin() override
        {
            m_logger->never("PositionDomain.getValue()");
            int val = m_state->getMinLedPosition();
            m_logger->never("\t%d",val);
            double dv = (double)val;
            return dv;
        }
    };

    class StepDomain : public AnimationDomain
    {
    };

    class AnimationRange
    {
    public:
        AnimationRange(double low, double high, bool unfold)
        {
            m_low = low;
            m_high = high;
            m_unfold = unfold;
            m_logger = &AnimationLogger;
        }

        double getValue(double position)
        {
            m_logger->never("AnimationRange.getValue(%f)  %f-%f",position,m_low,m_high);
            if (position <= 0 || m_high == m_low)
            {
                return m_low;
            }
            if (position >= 1)
            {
                return m_high;
            }
            double diff = m_high - m_low;
            double value = m_low + position * diff;
            return value;
        }

        double getLow() { return m_low; }
        double getHigh() { return m_high; }

    private:
        double m_low;
        double m_high;
        bool m_unfold;
        Logger* m_logger;
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
            m_logger->never("Linear ease %f",position);
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
        }

        double get(AnimationRange &range)
        {
            m_logger->never("Animator.get()");
            double position = m_domain.getPosition();
            double ease = m_ease.calculate(position);
            //m_logger->never("\tpos %f.  ease %f",position,ease);
            double result = range.getValue(ease);
            //m_logger->never("\tpos %f.  ease %f. result %f.  ",result);
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
            m_logger = &ScriptLogger;
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

        double getTimeValue(ScriptState &state, double low, double high)
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
            m_logger->never("\ttime animator value %f",value);
            return value;
            */
            return 0;
        }

        double getPositionValue(ScriptState &state, double low, double high, double percent)
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

        void setUnfold(IScriptValue *unfold)
        {
            delete m_unfoldValue;
            m_unfoldValue = unfold;
        }

        bool isUnfolded(IScriptCommand *cmd)
        {
            return m_unfoldValue && m_unfoldValue->getBoolValue(cmd, false);
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