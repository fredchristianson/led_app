#ifndef DR_ANIMATION_H
#define DR_ANIMATION_H

#include "../logger.h"
#include "./script_interface.h"

namespace DevRelief {
    Logger AnimationLogger("Animation",ANIMATION_LOGGER_LEVEL);
    class Animator {
        public:
        Animator(double domainStart, double domainEnd, double rangeLow, double rangeHigh, bool unfoldRange) {
            m_logger = &AnimationLogger;
            m_domainStart = domainStart;
            m_domainEnd = domainEnd;
            m_rangeLow = rangeLow;
            m_rangeHigh = rangeHigh;
            m_unfoldRange = unfoldRange;
            m_negate = domainStart > domainEnd;
            if (m_negate) {
                m_domainEnd = domainStart;
                m_domainStart = domainEnd;
            }
            m_logger->debug("create Animator on base %f-%f. %s",domainStart,domainEnd,unfoldRange?"unfold" : "");
        }

/* old 
        double getValuePercent(double pos) {
            if (pos <= m_domainStart) { return 0;}
            if (pos >= m_domainEnd) { return m_unfoldRange ? 0 : 1.0;}
            if (m_domainStart == m_domainEnd) { return 0;}
            double pct = 0;
            if (!m_unfoldRange) {
                pct = (pos - m_domainStart)/(m_domainEnd-m_domainStart);
            } else {
                pct = (pos - m_domainStart)/(m_domainEnd-m_domainStart);
                if (pct<=0.5) {
                    pct = pct * 2;
                } else {
                    pct = 1 - 2*(pct-0.5);
                }
            }
            m_logger->debug("\tgot Animator pct %f=%f (base %f-%f)",pos,pct,m_domainStart,m_domainEnd);
            return pct;
        }

        double getValueAt(double low, double high, double pos) {
            double pct = getValuePercent(pos);
            return getValueAtPercent(low,high,pct);
        }

        double getValueAtPercent(double low, double high, double percent) {
            double pct = percent;
            double range = high-low;
            double value = low + pct*range;
            m_logger->debug("\tgetValueAtPercent %f=%f+%f*%f",value,low,pct,range);
            return value;
        }
*/
        protected:
            virtual double ease(double pos) {
                return pos;
            }
        private:
            double m_domainStart;
            double m_domainEnd;
            double m_rangeLow;
            double m_rangeHigh;
            bool m_negate;
            bool m_unfoldRange;
            Logger* m_logger;
            // easing

            /*
            function easeInOutCubic(x: number): number {
                return x < 0.5 ? 4 * x * x * x : 1 - pow(-2 * x + 2, 3) / 2;
                }
            */
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
                m_unfold = false;
            }

            virtual ~ValueAnimator() {
                delete m_speedValue;
                delete m_durationValue;
                delete m_repeatValue;
                delete m_repeatDelayValue;
                delete m_unfoldValue;
            };

            void destroy() override { delete this;}

            void setSpeed(IScriptValue* speed) { delete m_speedValue ; m_speedValue = speed;}
            void setDuration(IScriptValue* duration) { delete m_durationValue ; m_durationValue = duration;}
            void setRepeat(IScriptValue* repeat) { delete m_repeatValue ; m_repeatValue = repeat;}
            void setRepeatDelay(IScriptValue* repeatDelay) { delete m_repeatDelayValue ; m_repeatDelayValue = repeatDelay;}
            

            double getTimeValue(ScriptState& state, double low, double high) {
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

            double getPositionValue(ScriptState&state,double low, double high,double percent) {
                /*
                m_logger->debug("getPositionValue %f %f %f",low,high,percent);
                Animator animate(low,high,isUnfolded(state));
                double val = animate.getValueAtPercent(low,high,percent);
                m_logger->debug("\tposition animator value %f",val);
                return val;
                */
               return 0;
            }

            void setUnfold(IScriptValue* unfold) {
                delete m_unfoldValue;
                m_unfoldValue = unfold;
            }

            bool isUnfolded(IScriptCommand* cmd) {
                return m_unfoldValue && m_unfoldValue->getBoolValue(cmd,false);
            }

            bool hasSpeedOrDuration(IScriptCommand*cmd) {
                return (m_speedValue && m_speedValue->getIntValue(cmd,0)>0)||
                        (m_durationValue && m_durationValue->getIntValue(cmd,0)>0);
            }

            bool canAnimateOverTime(IScriptCommand* cmd) { 
                m_logger->test("canAnimateOverTime");
                bool speedOrDuration = hasSpeedOrDuration(cmd);
                m_logger->test("\t%s",speedOrDuration?"yes":"no");
                return speedOrDuration;
            }
        protected:
            IScriptValue* m_speedValue; 
            IScriptValue* m_durationValue;  // ignored if m_seed is set
            IScriptValue*    m_repeatValue;
            IScriptValue*    m_repeatDelayValue;
            IScriptValue*    m_unfoldValue;
            bool  m_unfold;
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
            void destroy() override { delete this;}
    };

}

#endif