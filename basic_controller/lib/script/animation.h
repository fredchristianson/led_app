#ifndef DR_ANIMATION_H
#define DR_ANIMATION_H

#include "../logger.h"

namespace DevRelief {
    Logger AnimationLogger("Animation",ANIMATION_LOGGER_LEVEL);
    class Animator {
        public:
        Animator(double start, double end, bool unfold = false) {
            m_logger = &AnimationLogger;
            m_start = start;
            m_end = end;
            m_unfold = unfold;
            m_negate = start > end;
            if (m_negate) {
                m_end = start;
                m_start = end;
            }
            m_logger->test("create Animator on base %f-%f",start,end);
        }

        double getValuePercent(double pos) {
            if (pos <= m_start) { return 0;}
            if (pos >= m_end) { return m_unfold ? 0 : 1.0;}
            if (m_start == m_end) { return 0;}
            double pct = 0;
            if (!m_unfold) {
                pct = (pos - m_start)/(m_end-m_start);
            } else {
                pct = (pos - m_start)/(m_end-m_start);
                if (pct<=0.5) {
                    pct = pct * 2;
                } else {
                    pct = 1 - 2*(pct-0.5);
                }
            }
            m_logger->test("\tgot Animator pct %f=%f (base %f-%f)",pos,pct,m_start,m_end);
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
            m_logger->test("getValueAtPercent %f=%f+%f*%f",value,low,pct,range);
            return value;
        }
        private:
            double m_start;
            double m_end;
            bool m_negate;
            bool m_unfold;
            Logger* m_logger;
            // easing
    };


}

#endif