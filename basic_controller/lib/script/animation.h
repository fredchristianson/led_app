#ifndef DR_ANIMATION_H
#define DR_ANIMATION_H

#include "../logger.h"

namespace DevRelief {
    Logger AnimationLogger("Animation",ANIMATION_LOGGER_LEVEL);
    class Animator {
        public:
        Animator(double start, double end) {
            m_logger = &AnimationLogger;
            m_start = start;
            m_end = end;
            m_negate = start > end;
            if (m_negate) {
                m_end = start;
                m_start = end;
            }
            m_logger->test("create Animator on base %f-%f",start,end);
        }

        double getValuePercent(double pos) {
            if (pos <= m_start) { return 0;}
            if (pos >= m_end) { return 1.0;}
            if (m_start == m_end) { return 0;}
            double pct = (pos - m_start)/(m_end-m_start);
            m_logger->test("\tgot Animator pct %f=%f (base %f-%f)",pos,pct,m_start,m_end);
            return pct;
        }

        double getValueAt(double low, double high, double pos) {
            double pct = getValuePercent(pos);
            double range = high-low;
            double value = low + pct*range;
            m_logger->test("\tgot Animator value %f from %f in %f-%f",value,pos,low,high);
            return value;
        }
        private:
            double m_start;
            double m_end;
            bool m_negate;
            Logger* m_logger;
            // easing
    };


}

#endif