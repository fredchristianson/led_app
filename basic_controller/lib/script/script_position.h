#ifndef DRSCRIPT_POSITION_H
#define DRSCRIPT_POSITION_H

#include "../logger.h";
#include "../led_strip.h";
#include "./script_interface.h";
#include "./script_value.h";
#include "./animation.h";

namespace DevRelief
{

    class ScriptPosition 
    {
    public:
        ScriptPosition()
        {
            ScriptLogger.debug("create ScriptPosition()");
            m_startValue = NULL;
            m_endValue = NULL;
            m_countValue = NULL;
            m_skipValue = NULL;
            m_unit = POS_PERCENT;
            m_type = POS_RELATIVE;
            m_previous = NULL;
            m_wrapValue = NULL;
            m_reverseValue = NULL;
            m_start = 0;
            m_count = 0;
            m_wrap = true;
            m_reverse = false;
            m_logger = &ScriptLogger;
            m_animation = NULL;
            m_animationOffset = 0;
        }

        ~ScriptPosition() {
            delete m_startValue;
            delete m_countValue;
            delete m_endValue;
            delete m_skipValue;
            delete m_wrapValue;
            delete m_reverseValue;
            delete m_animation;
        }

        void destroy() {
            delete this;
        }

        void updateValues(ScriptState &state, IHSLStrip *strip);
        IHSLStrip *getPrevious()
        {
            return m_previous;
        }

        void setStartValue(IScriptValue *val) { m_startValue = val; }
        void setCountValue(IScriptValue *val) { m_countValue = val; }
        void setEndValue(IScriptValue *val) { m_endValue = val; }
        void setSkipValue(IScriptValue *val) {
            if (val != NULL) {
                delete m_skipValue;
                m_logger->debug("got skip value %s",val->toString().get());
                m_skipValue = val; 
            }
        }
        void setUnit(PositionUnit unit) { m_unit = unit; }
        void setHue(int index, int16_t hue, HSLOperation op = REPLACE)
        {
            if (m_previous == NULL || !translate(index)) {
                //ScriptLogger.periodic(ERROR_LEVEL,100,NULL,"Script position missing a previous strip");
                return;
            }
            m_previous->setHue((index), hue, op);
        }
        void setSaturation(int index, int16_t saturation, HSLOperation op = REPLACE)
        {
            if (m_previous == NULL || !translate(index)) {
                //ScriptLogger.periodic(ERROR_LEVEL,100,NULL,"Script position missing a previous strip");
                return;
            }
            m_previous->setSaturation((index), saturation, op);
        }
        void setLightness(int index, int16_t lightness, HSLOperation op = REPLACE)
        {
            if (m_previous == NULL || !translate(index)) {
                //ScriptLogger.periodic(ERROR_LEVEL,100,NULL,"Script position missing a previous strip");
                return;
            }
            m_previous->setLightness((index), lightness, op);
        }
        void setRGB(int index, const CRGB &rgb, HSLOperation op = REPLACE)
        {
            int orig = index;
            if (m_previous == NULL || !translate(index)) {
                //ScriptLogger.periodic(ERROR_LEVEL,1000,NULL,"Script position missing a previous strip");
                return;
            }
            m_logger->never("\ttranslated RGB index %d===>%d",orig,index);
            m_previous->setRGB((index), rgb, op);
        }
        size_t getCount() { return m_count; }
        size_t getStart() { return m_start; }
        bool translate(int& index)
        {
            m_logger->test("translate %d.  start=%d. count=%d. end=%d. skip=%d this=0x%04X anim:%d",index,m_start,m_count,m_end,m_skip,this,m_animationOffset);
            if (m_count == 0) { return false;}
            index = index + m_animationOffset;
            if (m_skipValue) {
                m_logger->test("\tskip %d. index=%d",m_skip,index*m_skip);
                index = index * m_skip;
            } else {
                m_logger->test("\tno skip");
            }
            if (index < 0) {
                if (m_wrap) {
                    index = m_count + (index%m_count);
                } else {
                    m_logger->test("clip");
                    return false;
                }
            }
            if (index >= m_count) {
                if (m_wrap) {
                    index = index%m_count;
                } else {
                    m_logger->test("clip");
                    return false;
                }
            }

            // always works in PIXEL units.  updateValues took care of % to pixel if needed

            if (m_start>m_end) {
                index = -index;

            }
            index = m_start+index;
            m_logger->test("\ttranslated %d",index);

            return true;
        }

        void setWrap(IScriptValue* wrap) { m_wrapValue = wrap; }
        void setReverse(IScriptValue* reverse) { m_reverseValue = reverse; }
        void clear() { m_previous->clear();}
        void show() { m_previous->show();}
        void setAnimator(PositionAnimator* animator) {
            m_logger->debug("delete old animator 0x%04x",m_animation);
            delete m_animation;
            m_logger->debug("set new animator");
            m_animation = animator;
            m_logger->debug("PositionAnimator 0x%04X",animator);
        }
    private:
        // values that may be variables
        IScriptValue* m_wrapValue;
        IScriptValue* m_reverseValue;
        IScriptValue *m_startValue;
        IScriptValue *m_endValue;
        IScriptValue *m_countValue;
        IScriptValue *m_skipValue;

        PositionAnimator* m_animation;
        int m_animationOffset;
        // evaluated variable values
        int m_start;
        int m_count;
        int m_end;
        bool m_wrap;
        bool m_reverse;
        int m_skip;
        PositionUnit m_unit;
        PositionType m_type;
        IHSLStrip *m_previous; // in ScriptCommand order
        IHSLStrip *m_base;      // may be different from previous for ABSOLUTE and other
        Logger* m_logger;
    };

  
}
#endif
