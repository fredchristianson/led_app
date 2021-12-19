#ifndef DRSCRIPT_POSITION_H
#define DRSCRIPT_POSITION_H

#include "../logger.h";
#include "../led_strip.h";
#include "./script_interface.h";
#include "./script_value.h";
#include "./animation.h";

namespace DevRelief
{

    class ScriptPosition : public IStripModifier
    {
    public:
        ScriptPosition()
        {
            ScriptLogger.debug("create ScriptPosition()");
            m_startValue = NULL;
            m_endValue = NULL;
            m_countValue = NULL;
            m_offsetValue = NULL;
            m_skipValue = NULL;
            m_unit = POS_PERCENT;
            m_type = POS_RELATIVE;
            m_previousStrip = NULL;
            m_wrapValue = NULL;
            m_reverseValue = NULL;
            m_start = 0;
            m_count = 0;
            m_offset = 0;
            m_wrap = true;
            m_reverse = false;
            m_logger = &ScriptLogger;
        }

        ~ScriptPosition() {
            delete m_startValue;
            delete m_countValue;
            delete m_endValue;
            delete m_skipValue;
            delete m_wrapValue;
            delete m_reverseValue;
            delete m_offsetValue;
        }

        void destroy() {
            delete this;
        }

        void setPositionType(PositionType t) { m_type = t;}
        void updateValues(IScriptCommand* cmd, IHSLStrip *previousStrip){
            m_previousStrip = previousStrip;
            if (previousStrip == NULL) {
                m_logger->error("ScriptPosition.updateValues needs a previous strip");
                return;
            }
            ScriptLogger.test("updateValues strip");
            if (m_wrapValue) {
                m_wrap = m_wrapValue->getBoolValue(cmd,true);
            } else {
                m_wrap = true;
            }
            if (m_reverseValue) {
                m_reverse = m_reverseValue->getBoolValue(cmd,true);
            } else {
                m_reverse = false;
            }
            if (m_skipValue) {
                int skip  = m_skipValue->getIntValue(cmd,1);
                m_skip = skip;
                ScriptLogger.test("skip %d %d.  %s",skip,m_skip,m_skipValue->toString().get());
            } else {
                m_skip = 1;
            }
            
            if (m_offsetValue) {
                int offset  = m_offsetValue->getIntValue(cmd,1);
                m_offset = offset;
                ScriptLogger.test("offset %d %d.  %s",offset,m_offset,m_offsetValue->toString().get());
            } else {
                m_offset = 1;
            }

            if (m_type == POS_ABSOLUTE){
                m_previousStrip = previousStrip->getFirstHSLStrip();
            }
            m_start = 0;
            if (m_startValue != NULL)
            {
                m_start = m_startValue->getIntValue(cmd,  m_start);
            }
            m_count = m_previousStrip->getCount();
            if (m_countValue != NULL)
            {
                m_count = m_countValue->getIntValue(cmd,  m_count);
                m_end = m_start+m_count-1;
            }
            else if (m_endValue != NULL)
            {
                m_end = m_start + m_count-1;
                m_end = m_endValue->getIntValue(cmd, m_end);
                m_count = abs(m_end - m_start)+1;
            };
            if(m_reverse) {
                int tmp = m_start;
                m_start = m_end;
                m_end = tmp;
            }
            if (m_unit == POS_PERCENT) {
                double baseCount = m_previousStrip->getCount();
               // ScriptLogger.debug("PERCENT: base: %f. start: %d. count %d. end %d. skip %d. wrap: %s.  reverse: %s.",baseCount, m_start,m_count,m_end,m_skip,(m_wrap?"true":"false"),(m_reverse?"true":"false"));
                m_start = round(m_start*baseCount/100.0);
                m_end = round(m_end*baseCount/100.0);
                m_count = round(m_count*baseCount/100.0);
              //  ScriptLogger.debug("\tadjusted: base: %f. start: %d. count %d. end %d. skip %d. wrap: %s.  reverse: %s.",baseCount, m_start,m_count,m_end,m_skip,(m_wrap?"true":"false"),(m_reverse?"true":"false"));
            }

            ScriptLogger.never("\tstart: %d. count %d. end %d. skip %d. wrap: %s.  reverse: %s.",m_start,m_count,m_end,m_skip,(m_wrap?"true":"false"),(m_reverse?"true":"false"));
        }

        IHSLStrip *getBase()
        {
            return m_previousStrip;
        }

        void setStartValue(IScriptValue *val) { m_startValue = val; }
        void setCountValue(IScriptValue *val) { m_countValue = val; }
        void setEndValue(IScriptValue *val) { m_endValue = val; }
        void setSkipValue(IScriptValue *val) {
            if (val != NULL) {
                delete m_skipValue;
                m_logger->never("got skip value %s",val->toString().get());
                m_skipValue = val; 
            } else {
                m_logger->never("skip is NULL");
            }
        }
        void setUnit(PositionUnit unit) { m_unit = unit; }
        void setHue(int index, int16_t hue, HSLOperation op = REPLACE)
        {
            if (m_previousStrip == NULL || !translate(index)) {
                //ScriptLogger.periodic(ERROR_LEVEL,100,NULL,"Script position missing a previous strip");
                return;
            }
            m_previousStrip->setHue((index), hue, op);
        }
        void setSaturation(int index, int16_t saturation, HSLOperation op = REPLACE)
        {
            if (m_previousStrip == NULL || !translate(index)) {
                //ScriptLogger.periodic(ERROR_LEVEL,100,NULL,"Script position missing a previous strip");
                return;
            }
            m_previousStrip->setSaturation((index), saturation, op);
        }
        void setLightness(int index, int16_t lightness, HSLOperation op = REPLACE)
        {
            if (m_previousStrip == NULL || !translate(index)) {
                //ScriptLogger.periodic(ERROR_LEVEL,100,NULL,"Script position missing a previous strip");
                return;
            }
            m_previousStrip->setLightness((index), lightness, op);
        }
        void setRGB(int index, const CRGB &rgb, HSLOperation op = REPLACE)
        {
            int orig = index;
            if (m_previousStrip == NULL || !translate(index)) {
                //ScriptLogger.periodic(ERROR_LEVEL,1000,NULL,"Script position missing a previous strip");
                return;
            }
          //  m_logger->never("\ttranslated RGB index %d===>%d",orig,index);
            m_previousStrip->setRGB((index), rgb, op);
        }
        size_t getCount() { return m_count; }
        size_t getStart() { return m_start; }
        bool translate(int& index)
        {
            // never works in PIXEL units.  updateValues took care of % to pixel if needed

            if (m_count == 0) { return false;}
            if (m_skipValue) {
                m_logger->never("\tskip %d. index=%d",m_skip,index*m_skip);
                index = index * m_skip;
            } else {
                m_logger->test("\tno skip");
            }
            index = index + m_offset;
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


            if (m_start>m_end) {
                index = -index;

            }
            index = m_start+index;

            return true;
        }

        void setWrap(IScriptValue* wrap) { m_wrapValue = wrap; }
        void setReverse(IScriptValue* reverse) { m_reverseValue = reverse; }
        void setOffset(IScriptValue* offset) { m_offsetValue = offset; }
        void clear() { m_previousStrip->clear();}
        void show() { m_previousStrip->show();}


        IHSLStrip* getFirstHSLStrip() { return m_previousStrip ? m_previousStrip->getFirstHSLStrip() : NULL;}
    private:
        // values that may be variables
        IScriptValue* m_wrapValue;
        IScriptValue* m_reverseValue;
        IScriptValue *m_startValue;
        IScriptValue *m_endValue;
        IScriptValue *m_countValue;
        IScriptValue *m_skipValue;
        IScriptValue *m_offsetValue;

        int m_animationOffset;
        // evaluated variable values
        int m_start;
        int m_count;
        int m_end;
        bool m_wrap;
        bool m_reverse;
        int m_skip;
        int m_offset;
        PositionUnit m_unit;
        PositionType m_type;
        IHSLStrip *m_previousStrip;  // may be previous command,  ABSOLUTE, specific strip, etc
        Logger* m_logger;
    };

  
}
#endif
