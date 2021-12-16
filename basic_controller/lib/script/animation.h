#ifndef DR_ANIMATION_H
#define DR_ANIMATION_H


namespace DevRelief {
    class Animator {
        public:
        Animator(double start, double end) {
            m_start = start;
            m_end = end;
            m_negate = start > end;
            if (m_negate) {
                m_end = start;
                m_start = end;
            }
        }

        double getValuePercent(double pos) {
            if (pos <= m_start) { return 0;}
            if (pos >= m_end) { return 1.0;}
            if (m_start == m_end) { return 0;}
            double pct = (pos - m_start)/(m_end-m_start);
            return pct;
        }
        private:
            double m_start;
            double m_end;
            bool m_negate;
            // easing
    };


}

#endif